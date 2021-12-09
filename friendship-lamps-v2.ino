// #define BLYNK_TEMPLATE_ID "TMPLK4ed64ko"
// #define BLYNK_DEVICE_NAME "Friendship Lamps"

#define THING_INDEX 1

#if THING_INDEX == 0
#include "conf_0.h"
#else
#include "conf_1.h"
#endif

#include <FastLED.h>
#define LEDOUTPUT_TICK 10

#include <WiFi.h>
#include <WiFiManager.h>

#include <EEPROM.h>
#define EEPROM_SIZE 1 // bytes
#define EEPROM_ADDR_BRIGHTNESS_INDEX 0

#include <BlynkSimpleEsp32.h>
WidgetBridge blynkBridge(VPIN_COLOR_SEND);
BlynkTimer timer_sendToOtherDevice;
#define SEND_TO_OTHER_DEVICE_TICK 1000
#define FORCE_SEND_TO_OTHER_DEVICE_TICK 30000 // send to other device regardless of state every 30 seconds
uint32_t forceSend_lastMillis = 0;
BlynkTimer timer_pollBtns;
BlynkTimer timer_ledOutputs;
#define EFFECT_UPDATE_TICK 10
BlynkTimer timer_updateEffect;
#define BLYNK_STATUS_REQUEST_COLOR 0
#define BLYNK_STATUS_MORSECODE_PULSE_ON 1
#define BLYNK_STATUS_MORSECODE_PULSE_OFF 2
#define BLYNK_STATUS_SWITCH_TO_NIGHTLIGHT 3
#define BLYNK_STATUS_SWITCH_TO_NORMAL 4
bool morsecode_pulse = false;
#define MORSECODE_MIN_PULSE 50    // ms
#define MORSECODE_MAX_PULSE 20000 // 20 s
uint32_t morsecode_send_lastPulseMillis = 0;
uint32_t morsecode_show_lastPulseMillis = 0;

#define LEDTYPE WS2812B
#define NUMLEDS 16
// only 16 PWM channels -- so max of 16 white LED channels
#define NUMWHITE 8

#define LEDPIN 22
uint8_t WHITEPINS[] = {25, 26, 32, 33, 27, 14, 12, 13};

// unpressed HIGH, pressed LOW
#define BTN_FRONT 10
#define BTN_SIDE_SINGLE 9
#define BTN_SIDE_THREE_FRONT 5
#define BTN_SIDE_THREE_MIDDLE 18
#define BTN_SIDE_THREE_BACK 23
#define BTN1PIN_BRIGHTNESS BTN_FRONT
#define BTN2PIN_COLOR BTN_SIDE_THREE_FRONT
#define BTN3PIN_NIGHTLIGHT BTN_SIDE_SINGLE
#define BTN4PIN_EFFECT BTN_SIDE_THREE_MIDDLE
#define BTN5PIN_MORSECODE BTN_SIDE_THREE_BACK
#define BTN_TICK 1 // tick time 1 ms
uint32_t btn_previousTickTime;
#define BTN_MAX_BOUNCE 15 // ms
uint16_t btn1_counter = 0;
uint16_t btn2_counter = 0;
uint16_t btn3_counter = 0;
uint16_t btn4_counter = 0;
uint16_t btn5_counter = 0;
uint16_t btn5_offcounter = 0;

#define LED_PWM_FREQUENCY 5000 /* Hz */
#define LED_PWM_RESOLUTION 8   /* pwm resolution in bits */

extern const uint8_t gammaVals[];

// front arrays to hold current output
CRGB leds[NUMLEDS];
uint8_t whiteleds[NUMWHITE];
// back arrays to hold last output from other mode
CRGB previous_leds[NUMLEDS];
uint8_t previous_whiteleds[NUMWHITE];

#define NUM_BRIGHTNESS_PRESETS 3
uint8_t brightnessPresets[] = {64, 128, 192};
uint8_t morsecode_pulseBrightness[] = {128, 192, 255};
uint8_t brightnessPresetsNightlight[] = {64, 128, 255};
uint8_t currentBrightnessPresetIndex = 0;

#define GLOBALMODE_NORMAL 0
#define GLOBALMODE_NIGHTLIGHT 1
uint8_t globalMode = GLOBALMODE_NORMAL; // start in normal mode on power on

typedef struct {
  CRGB rgb;
  uint8_t w;
} RGBW;

#define NUM_COLOR_PRESETS 8
RGBW colorPresets[] = {
  {CRGB(255, 30, 5), 0},
  {CRGB(244, 128, 22), 0},
  {CRGB(255, 239, 26), 0},
  {CRGB(12, 255, 38), 0},
  {CRGB(37, 250, 236), 0},
  {CRGB(4, 137, 244), 0},
  {CRGB(144, 0, 255), 0},
  {CRGB(255, 14, 241), 0},
};
uint8_t currentColorPresetIndex = 0;
uint8_t lastSentColorPresetIndex = currentColorPresetIndex;
bool usingCustomColor = false;
RGBW customColor = {CRGB(0,0,0),0};

// RGBW currentOutputColor;
// uint32_t ledUpdate_lastTickTime = 0;
// #define LEDUPDATE_TICK 100 // ms

RGBW nightlightColor = {CRGB(0, 0, 0), 192};

#define COLOR_OFF \
{ CRGB(0, 0, 0), 0 }

#define NUM_EFFECTS 8
// effect ids must be numbered sequentially from 0
#define EFFECT_NORMAL 0

#define EFFECT_TWINKLE 1
#define EFFECT_TWINKLE_SLOW 2
#define EFFECT_TWINKLE_FLASH 3
uint8_t twinkle_ledOffsets[NUMLEDS];
uint8_t twinkle_step = 0;
#define TWINKLE_STEP_INCREMENT_PER_TICK 3
#define TWINKLE_SLOW_STEP_INCREMENT_PER_TICK 1
#define TWINKLE_FLASH_STEP_INCREMENT_PER_TICK 3
#define TWINKLE_FLASH_LED_ON_CUTOFF 192

#define EFFECT_DISCO 4
#define EFFECT_DISCO_SLOW 5
#define EFFECT_DISCO_TWINKLE 6
#define EFFECT_DISCO_FLASH 7
uint8_t disco_currentColor = 0;
uint8_t disco_currentInterpolation = 0;
// how much to increment interpolation by every time we update
// higher = faster
#define DISCO_INTERPOLATION_INCREMENT_PER_TICK 8
#define DISCO_SLOW_INTERPOLATION_INCREMENT_PER_TICK 2
uint8_t currentEffect = EFFECT_NORMAL;
uint8_t lastSentEffect = currentEffect;

bool isWifiEnabled = true;

void setup() {
  // random seed from floating analog pin
  random16_set_seed(analogRead(35));

  // start eeprom
  EEPROM.begin(EEPROM_SIZE);

  // configure pin inputs
  pinMode(BTN1PIN_BRIGHTNESS, INPUT);
  pinMode(BTN2PIN_COLOR, INPUT);
  pinMode(BTN3PIN_NIGHTLIGHT, INPUT);
  pinMode(BTN4PIN_EFFECT, INPUT);
  pinMode(BTN5PIN_MORSECODE, INPUT);

  // set up a separate PWM channel for each white LED
  for (uint8_t ledIndex = 0; ledIndex < NUMWHITE; ledIndex++) {
    // configure PWM channel for LED i
    ledcSetup(ledIndex /* PWM channel number */, LED_PWM_FREQUENCY, LED_PWM_RESOLUTION);
    // attach the PWM channel to the LED pin
    ledcAttachPin(WHITEPINS[ledIndex] /* pin number */, ledIndex /* PWM channel number */);
  }

  // setup rgb leds
  FastLED.addLeds<LEDTYPE, LEDPIN, GRB>(leds, NUMLEDS);
  FastLED.setDither(0);

  // initial brightness from eeprom
  currentBrightnessPresetIndex = EEPROM.read(EEPROM_ADDR_BRIGHTNESS_INDEX);
  FastLED.setBrightness(pgm_read_byte(&gammaVals[brightnessPresets[currentBrightnessPresetIndex]]));

  // check if wifi should be disabled
#define WIFI_DISABLE_BTN BTN_FRONT
#define INDICATOR_COLOR_WIFI_DISABLED { CRGB(0, 0, 255), 0 }
#define WIFI_RESET_BTN BTN_SIDE_SINGLE
#define INDICATOR_COLOR_WIFI_RESET { CRGB(0, 255, 0), 0 }
  bool resetWifi = false;
  if (digitalRead(WIFI_DISABLE_BTN) == LOW) {
    delay(250);
    if (digitalRead(WIFI_DISABLE_BTN) == LOW) {
      isWifiEnabled = false;
      // flash indicator colour
      fillSolid_RGBW(INDICATOR_COLOR_WIFI_DISABLED);
      FastLED.show();
      delay(250);
      fillSolid_RGBW(COLOR_OFF);
      FastLED.show();
      delay(250);

      fillSolid_RGBW(INDICATOR_COLOR_WIFI_DISABLED);
      FastLED.show();
      delay(250);
      fillSolid_RGBW(COLOR_OFF);
      FastLED.show();
      delay(250);

      fillSolid_RGBW(INDICATOR_COLOR_WIFI_DISABLED);
      FastLED.show();
      delay(250);
      fillSolid_RGBW(COLOR_OFF);
      FastLED.show();
      delay(250);

      // wait for btn to be released
      while (digitalRead(WIFI_DISABLE_BTN) == LOW) {
        delay(20);
      }
    }
  } else if (digitalRead(WIFI_RESET_BTN) == LOW) {
    delay(250);
    if (digitalRead(WIFI_RESET_BTN) == LOW) {
      resetWifi = true;
      fillSolid_RGBW(INDICATOR_COLOR_WIFI_RESET);
      FastLED.show();
      delay(350);
      fillSolid_RGBW(COLOR_OFF);
      FastLED.show();
    }
  }

  // connect to wifi
  if (isWifiEnabled) {
    // connecting indicator
    fillSolid_RGBW({CRGB(255, 255, 255), 0});
    FastLED.show();

    // connect to wifi
    WiFiManager wifiManager;
    
    if (resetWifi) {
      wifiManager.resetSettings();
    }

    // try to connect to saved ssid/password
    // else start AP for configuration
    if (!wifiManager.autoConnect(WIFIMANAGER_SSID, WIFIMANAGER_PASSWORD)) {
      delay(3000);
      // reset and try again if it didn't work
      ESP.restart();
      delay(5000);
    }
    // now connected to wifi

    // blynk configuration
    delay(1000);
    Blynk.config(BLYNK_AUTH_THIS);
    bool success = Blynk.connect(180);
    if (!success) {
      ESP.restart();
      delay(5000);
    }
  }

  // setup timers
  timer_sendToOtherDevice.setInterval(SEND_TO_OTHER_DEVICE_TICK, timerEvent_sendToOtherDevice);
  timer_pollBtns.setInterval(BTN_TICK, timerEvent_pollBtns);
  timer_ledOutputs.setInterval(LEDOUTPUT_TICK, showAllRGBW);
  timer_updateEffect.setInterval(EFFECT_UPDATE_TICK, timerEvent_updateEffect);

  // initialise offsets for twinkle effect
  for (uint8_t led = 0; led < NUMLEDS; led++) {
    twinkle_ledOffsets[led] = random8();
  }

  // request color from other device (sync with other device on power on)
  blynkBridge.virtualWrite(VPIN_STATUS_SEND, BLYNK_STATUS_REQUEST_COLOR);
}

void loop() {
  if (isWifiEnabled) {
    bool wifiSuccess = false;
    while (!wifiSuccess) {
      wifiSuccess = ensureWifiConnected();
    }
    Blynk.run();
    timer_sendToOtherDevice.run();
  }
  timer_pollBtns.run();
  timer_updateEffect.run();
  timer_ledOutputs.run();
}

void setRGBWPixel(uint8_t i, RGBW color) {
  if (i < NUMLEDS) {
    leds[i] = color.rgb;
  }
  if (i < NUMWHITE) {
    whiteleds[i] = color.w;
  }
}

void setRGBWPixelScaled(uint8_t i, RGBW color, uint8_t scaleTo) {
  if (i < NUMLEDS) {
    leds[i] = color.rgb;
    leds[i].nscale8(scaleTo);
  }
  if (i < NUMWHITE) {
    whiteleds[i] = (uint8_t)((double)color.w * (scaleTo / 255.0));
  }
}

// output leds from whiteleds[] array
void whiteLedsShow() {
  // apply the same brightness level as the RGB strip
  uint8_t currentBrightness = brightnessPresets[currentBrightnessPresetIndex];
  uint8_t gammaCorrectedCurrentBrightness = pgm_read_byte(&gammaVals[currentBrightness]);
  for (int i = 0; i < NUMWHITE; i++) {
    // read value for this LED from array
    // and update LED output value
    ledcWrite(i, (whiteleds[i] * gammaCorrectedCurrentBrightness) / 255);
  }
}

void showAllRGBW() {
  // set correct brightness
  if (shouldShowMorsecodePulse()) {
    FastLED.setBrightness(pgm_read_byte(&gammaVals[morsecode_pulseBrightness[currentBrightnessPresetIndex]]));
  } else {
    setBrightnessToCurrentlySelectedLevel();
  }
  FastLED.show();
  whiteLedsShow();
}

// set brightness specified by current brightness level
void setBrightnessToCurrentlySelectedLevel() {
  uint8_t brightness = (globalMode == GLOBALMODE_NIGHTLIGHT ? brightnessPresetsNightlight : brightnessPresets)[currentBrightnessPresetIndex];
  FastLED.setBrightness(pgm_read_byte(&gammaVals[brightness]));
}

bool shouldShowMorsecodePulse() {
  return (morsecode_pulse && (millis() - morsecode_show_lastPulseMillis) < MORSECODE_MAX_PULSE)      // pulse on and before max pulse length timeout
    || (!morsecode_pulse && (millis() - morsecode_show_lastPulseMillis) < MORSECODE_MIN_PULSE); // pulse off but haven't got to min pulse length yet so keep it on till then
}

// void updatePixels() {
//     // CRGB currentRGB = currentOutputColor.rgb;
//     // uint8_t currentW = currentOutputColor.w;
//     RGBW targetColor = colorPresets[currentColorPresetIndex];
//     RGBW newOutputColor = currentOutputColor;
//     if (currentOutputColor.rgb != targetColor.rgb && (millis() - ledUpdate_lastTickTime) > LEDUPDATE_TICK) {
//         if (currentOutputColor.rgb.r > targetColor.rgb.r) {
//             newOutputColor.rgb.r++;
//         } else if (currentOutputColor.rgb.r < targetColor.rgb.r) {
//             newOutputColor.rgb.r--;
//         }
//         if (currentOutputColor.rgb.g > targetColor.rgb.g) {
//             newOutputColor.rgb.g++;
//         } else if (currentOutputColor.rgb.g < targetColor.rgb.g) {
//             newOutputColor.rgb.g--;
//         }
//         if (currentOutputColor.rgb.b > targetColor.rgb.b) {
//             newOutputColor.rgb.b++;
//         } else if (currentOutputColor.rgb.b < targetColor.rgb.b) {
//             newOutputColor.rgb.b--;
//         }
//         if (currentOutputColor.w > targetColor.w) {
//             newOutputColor.w++;
//         } else if (currentOutputColor.w < targetColor.w) {
//             newOutputColor.w--;
//         }
//         ledUpdate_lastTickTime = millis();
//     }
//     fillSolid_RGBW(newOutputColor);
//     showAllRGBW();
// }

void fillSolid_RGBW(RGBW color) {
  for (uint8_t i = 0; i < NUMLEDS; i++) {
    leds[i] = color.rgb;
  }
  for (uint8_t i = 0; i < NUMWHITE; i++) {
    whiteleds[i] = color.w;
  }
  // currentOutputColor = color;
}

RGBW getCurrentColor() {
  if (usingCustomColor) return customColor;
  else return colorPresets[currentColorPresetIndex];
}

void timerEvent_updateEffect() {
  if (globalMode == GLOBALMODE_NIGHTLIGHT) {
    fillSolid_RGBW(nightlightColor);
    return;
  }
  switch (currentEffect) {
    default:
    case EFFECT_NORMAL: {
                          fillSolid_RGBW(getCurrentColor());
                          break;
                        }
    case EFFECT_TWINKLE:
    case EFFECT_TWINKLE_SLOW: {
                                RGBW color = getCurrentColor();
                                for (uint8_t led = 0; led < NUMLEDS; led++) {
                                  uint8_t thisLedOffset = twinkle_ledOffsets[led];
                                  uint8_t brightness = sin8(twinkle_step + thisLedOffset);
                                  setRGBWPixelScaled(led, color, brightness);
                                }
                                if (currentEffect == EFFECT_TWINKLE) {
                                  twinkle_step += TWINKLE_STEP_INCREMENT_PER_TICK;
                                } else {
                                  twinkle_step += TWINKLE_SLOW_STEP_INCREMENT_PER_TICK;
                                }
                                break;
                              }
    case EFFECT_TWINKLE_FLASH: {
                                 RGBW color = getCurrentColor();
                                 for (uint8_t led = 0; led < NUMLEDS; led++) {
                                   uint8_t thisLedOffset = twinkle_ledOffsets[led];
                                   uint8_t wave = triwave8(twinkle_step + thisLedOffset);
                                   if (wave >= TWINKLE_FLASH_LED_ON_CUTOFF) {
                                     setRGBWPixel(led, color);
                                   } else {
                                     setRGBWPixel(led, COLOR_OFF);
                                   }
                                 }
                                 twinkle_step += TWINKLE_FLASH_STEP_INCREMENT_PER_TICK;
                                 break;
                               }
    case EFFECT_DISCO:
    case EFFECT_DISCO_SLOW: {
                              RGBW currentDiscoColor = getCurrentDiscoColor();
                              fillSolid_RGBW(currentDiscoColor);
                              if (currentEffect == EFFECT_DISCO) {
                                incrementDiscoColor(DISCO_INTERPOLATION_INCREMENT_PER_TICK);
                              } else {
                                incrementDiscoColor(DISCO_SLOW_INTERPOLATION_INCREMENT_PER_TICK);
                              }
                              break;
                            }
    case EFFECT_DISCO_TWINKLE: {
                                 RGBW currentDiscoColor = getCurrentDiscoColor();
                                 for (uint8_t led = 0; led < NUMLEDS; led++) {
                                   uint8_t thisLedOffset = twinkle_ledOffsets[led];
                                   uint8_t brightness = sin8(twinkle_step + thisLedOffset);
                                   setRGBWPixelScaled(led, currentDiscoColor, brightness);
                                 }
                                 twinkle_step += TWINKLE_STEP_INCREMENT_PER_TICK;
                                 incrementDiscoColor(DISCO_INTERPOLATION_INCREMENT_PER_TICK);
                                 break;
                               }
    case EFFECT_DISCO_FLASH: {
                               RGBW currentDiscoColor = getCurrentDiscoColor();
                               for (uint8_t led = 0; led < NUMLEDS; led++) {
                                 uint8_t thisLedOffset = twinkle_ledOffsets[led];
                                 uint8_t wave = triwave8(twinkle_step + thisLedOffset);
                                 if (wave >= TWINKLE_FLASH_LED_ON_CUTOFF) {
                                   setRGBWPixel(led, currentDiscoColor);
                                 } else {
                                   setRGBWPixel(led, COLOR_OFF);
                                 }
                               }
                               twinkle_step += TWINKLE_FLASH_STEP_INCREMENT_PER_TICK;
                               incrementDiscoColor(DISCO_INTERPOLATION_INCREMENT_PER_TICK);
                               break;
                             }
  }
}

RGBW getCurrentDiscoColor() {
  return interpolateColors(
      colorPresets[disco_currentColor],
      colorPresets[disco_currentColor == NUM_COLOR_PRESETS - 1 ? 0 : disco_currentColor + 1], // check if at last colour, if so then wrap around
      disco_currentInterpolation);
}

void incrementDiscoColor(uint8_t increment) {
  if (disco_currentInterpolation <= 254 - increment) {
    disco_currentInterpolation += increment;
  } else {
    disco_currentColor++;
    if (disco_currentColor >= NUM_COLOR_PRESETS) {
      disco_currentColor = 0;
    }
    disco_currentInterpolation = 0;
  }
}

// interpolate between colours a and b
// ratio determines how far between the colours to interpolate; 0 is all the way to a, 255 is all the way to b
RGBW interpolateColors(RGBW a, RGBW b, uint8_t ratio) {
  double delta_r = (b.rgb.r - a.rgb.r) * ratio / 255;
  double delta_g = (b.rgb.g - a.rgb.g) * ratio / 255;
  double delta_b = (b.rgb.b - a.rgb.b) * ratio / 255;
  double delta_w = (b.w - a.w) * ratio / 255;
  return {CRGB(a.rgb.r + ((uint8_t)delta_r),
      a.rgb.g + ((uint8_t)delta_g),
      a.rgb.b + ((uint8_t)delta_b)),
         a.w + ((uint8_t)delta_w)};
}

void timerEvent_pollBtns() {
  // poll btn 1
  bool btn1_pressed = pollBtn(BTN1PIN_BRIGHTNESS, &btn1_counter);
  if (btn1_pressed) {
    advanceToNextBrightnessPreset();
  }

  // poll btn 2
  bool btn2_pressed = pollBtn(BTN2PIN_COLOR, &btn2_counter);
  if (btn2_pressed) {
    if (globalMode == GLOBALMODE_NORMAL) {
      advanceToNextColorPreset();
    }
  }

  // poll btn 3
  bool btn3_pressed = pollBtn(BTN3PIN_NIGHTLIGHT, &btn3_counter);
  if (btn3_pressed) {
    // action on btn 3 press
    toggleNightlightMode();
  }

  // poll btn 4
  bool btn4_pressed = pollBtn(BTN4PIN_EFFECT, &btn4_counter);
  if (btn4_pressed) {
    if (globalMode == GLOBALMODE_NORMAL) {
      advanceToNextEffect();
    }
  }

  // poll btn 5
  if (isWifiEnabled) {
    bool btn5_pressed = pollBtn(BTN5PIN_MORSECODE, &btn5_counter);
    bool btn5_unpressed = pollBtnForValue(BTN5PIN_MORSECODE, &btn5_offcounter, HIGH);
    if (btn5_pressed && morsecode_pulse == false) {
      blynkBridge.virtualWrite(VPIN_STATUS_SEND, BLYNK_STATUS_MORSECODE_PULSE_ON);
      morsecode_pulse = true;
      morsecode_send_lastPulseMillis = millis();
      morsecode_show_lastPulseMillis = millis();
    } else if (btn5_unpressed && morsecode_pulse == true && (millis() - morsecode_send_lastPulseMillis) > MORSECODE_MIN_PULSE) {
      blynkBridge.virtualWrite(VPIN_STATUS_SEND, BLYNK_STATUS_MORSECODE_PULSE_OFF);
      morsecode_pulse = false;
    }
  }
}

// increment btn counter every time we read it as active (LOW);
// reset to 0 every time we read as inactive;
// assume finished bouncing when counter is MAX_VAL (btn has been active for that long)
bool pollBtn(uint8_t btn_pin, uint16_t *btn_counter) {
  return pollBtnForValue(btn_pin, btn_counter, LOW);
}

bool pollBtnForValue(uint8_t btn_pin, uint16_t *btn_counter, uint8_t value) {
  uint8_t btn_value = digitalRead(btn_pin);
  if (btn_value == value) {
    (*btn_counter)++;
  } else {
    *btn_counter = 0;
  }

  return *btn_counter == BTN_MAX_BOUNCE;
}

void advanceToNextBrightnessPreset() {
  currentBrightnessPresetIndex++;
  if (currentBrightnessPresetIndex >= NUM_BRIGHTNESS_PRESETS) {
    currentBrightnessPresetIndex = 0;
  }
  writeBrightnessIndexToEEPROM();
}

void writeBrightnessIndexToEEPROM() {
  EEPROM.write(EEPROM_ADDR_BRIGHTNESS_INDEX, currentBrightnessPresetIndex);
  EEPROM.commit();
}

void advanceToNextColorPreset() {
  currentColorPresetIndex++;
  if (currentColorPresetIndex >= NUM_COLOR_PRESETS) {
    currentColorPresetIndex = 0;
  }
  fillSolid_RGBW(colorPresets[currentColorPresetIndex]);
  // showAllRGBW();
  usingCustomColor = false;
}

void toggleNightlightMode() {
  if (globalMode == GLOBALMODE_NORMAL) {
    globalMode = GLOBALMODE_NIGHTLIGHT;
    blynkBridge.virtualWrite(VPIN_STATUS_SEND, BLYNK_STATUS_SWITCH_TO_NIGHTLIGHT);
  } else {
    globalMode = GLOBALMODE_NORMAL;
    blynkBridge.virtualWrite(VPIN_STATUS_SEND, BLYNK_STATUS_SWITCH_TO_NORMAL);
  }

  // // swap front and back arrays
  // for (uint8_t i = 0; i < NUMLEDS; i++) {
  //     CRGB tmp = previous_leds[i];
  //     previous_leds[i] = leds[i];
  //     leds[i] = tmp;
  // }
  // for (uint8_t i = 0; i < NUMWHITE; i++) {
  //     uint8_t tmp = previous_whiteleds[i];
  //     previous_whiteleds[i] = whiteleds[i];
  //     whiteleds[i] = tmp;
  // }
}

void advanceToNextEffect() {
  currentEffect++;
  if (currentEffect >= NUM_EFFECTS) {
    currentEffect = 0;
  }
}

void timerEvent_sendToOtherDevice() {
  if (currentColorPresetIndex != lastSentColorPresetIndex
      || (millis() - forceSend_lastMillis) > FORCE_SEND_TO_OTHER_DEVICE_TICK) {
    blynk_sendToOtherDevice();
  }
  if (currentEffect != lastSentEffect
      || (millis() - forceSend_lastMillis) > FORCE_SEND_TO_OTHER_DEVICE_TICK) {
    blynk_sendEffectToOtherDevice();
  }
  blynk_updateAppColorLed(getCurrentColor());
}

// in separate function so we can send colour when requested, bypassing the
// check for if colour has changed
void blynk_sendToOtherDevice() {
  blynkBridge.virtualWrite(VPIN_COLOR_SEND, currentColorPresetIndex);
  lastSentColorPresetIndex = currentColorPresetIndex;
}

void blynk_sendEffectToOtherDevice() {
  blynkBridge.virtualWrite(VPIN_EFFECT_SEND, currentEffect);
  lastSentEffect = currentEffect;
}

void blynk_updateAppColorLed(RGBW color) {
  char hex[8];
  sprintf(hex, "#%X%X%X\0", color.rgb.r, color.rgb.g, color.rgb.b);
  String hexString = String(hex);
  Blynk.setProperty(VPIN_APP_COLOR_LED, "color", hexString);
  Blynk.virtualWrite(VPIN_APP_COLOR_LED, 255);
}

// runs when first connect to the API
BLYNK_CONNECTED() {
  // auth token of other device to connect to
  blynkBridge.setAuthToken(BLYNK_AUTH_OTHER);
}

// handle when we receive a color update from the other device
BLYNK_WRITE(VPIN_COLOR_READ) {
  currentColorPresetIndex = param.asInt();
  usingCustomColor = false;
}

// handle when we receive a status request from the other device
BLYNK_WRITE(VPIN_STATUS_READ) {
  uint8_t code = param.asInt();

  switch (code) {
    case BLYNK_STATUS_REQUEST_COLOR: {
                                       blynk_sendToOtherDevice();
                                       break;
                                     }
    case BLYNK_STATUS_MORSECODE_PULSE_ON: {
                                            morsecode_pulse = true;
                                            morsecode_show_lastPulseMillis = millis();
                                            break;
                                          }
    case BLYNK_STATUS_MORSECODE_PULSE_OFF: {
                                             morsecode_pulse = false;
                                             break;
                                           }
    case BLYNK_STATUS_SWITCH_TO_NIGHTLIGHT: {
                                              globalMode = GLOBALMODE_NIGHTLIGHT;
                                              break;
                                            }
    case BLYNK_STATUS_SWITCH_TO_NORMAL: {
                                          globalMode = GLOBALMODE_NORMAL;
                                          break;
                                        }
  }
}

// handle when we receive an effect update from the other device
BLYNK_WRITE(VPIN_EFFECT_READ) {
  currentEffect = param.asInt();
}

BLYNK_WRITE(VPIN_ZERGBA_READ) {
  customColor.rgb.r = param[0].asInt();
  customColor.rgb.g = param[1].asInt();
  customColor.rgb.b = param[2].asInt();

  usingCustomColor = true;
}

#define CONNECT_TIMEOUT 30 // seconds to wait to connect before booting local AP
#define AP_TIMEOUT 120     // seconds to wait in the config portal before trying again
bool ensureWifiConnected() {
  // do nothing if already connected
  if (WiFi.status() != WL_CONNECTED) {
    fillSolid_RGBW({CRGB(255,0,0), 0});
    showAllRGBW();
    WiFiManager wifiManager;

    wifiManager.setConnectTimeout(CONNECT_TIMEOUT);
    wifiManager.setTimeout(AP_TIMEOUT);

    // try to connect again; blocks until WiFi is connected, or time out
    wifiManager.autoConnect(WIFIMANAGER_SSID, WIFIMANAGER_PASSWORD);
  }

  // return false if still not connected, so we can try again
  return WiFi.status() == WL_CONNECTED;
}

// gamma correction values
const PROGMEM uint8_t gammaVals[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2,
  2, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5,
  5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10,
  10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
  17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
  25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
  37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
  51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
  69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
  90, 92, 93, 95, 96, 98, 99, 101, 102, 104, 105, 107, 109, 110, 112, 114,
  115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142,
  144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175,
  177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
  215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255};
