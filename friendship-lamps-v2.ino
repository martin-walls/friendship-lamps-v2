#include <FastLED.h>

#define LEDTYPE WS2812B
#define NUMLEDS 2
// only 16 PWM channels -- so max of 16 white LED channels
#define NUMWHITE 2

#define LEDPIN 22
uint8_t WHITEPINS[] = {25, 26};

// unpressed HIGH, pressed LOW
#define BTN1PIN 23
#define BTN2PIN 18
#define BTN3PIN 5
#define BTN_TICK 1 // tick time 1 ms
uint32_t btn_previousTickTime;
#define BTN_MAX_BOUNCE 15 // ms
uint8_t btn1_counter = 0;
uint8_t btn2_counter = 0;
uint8_t btn3_counter = 0;

#define LED_PWM_FREQUENCY 5000 /* Hz */
#define LED_PWM_RESOLUTION 8   /* pwm resolution in bits */

extern const uint8_t gammaVals[];

// front arrays to hold current output
CRGB leds[NUMLEDS];
uint8_t whiteleds[NUMWHITE];
// back arrays to hold last output from other mode
CRGB previous_leds[NUMLEDS];
uint8_t previous_whiteleds[NUMWHITE];



#define NUM_BRIGHTNESS_PRESETS 4
uint8_t brightnessPresets[] = {64, 128, 192, 255};
uint8_t currentBrightnessPresetIndex = 0;

#define GLOBALMODE_NORMAL 0
#define GLOBALMODE_NIGHTLIGHT 1
uint8_t globalMode = GLOBALMODE_NORMAL; // start in normal mode on power on

typedef struct {
    CRGB rgb;
    uint8_t w;
} RGBW;


#define NUM_COLOR_PRESETS 4
RGBW colorPresets[] = {
    {CRGB(255,0,0), 0},
    {CRGB(127,127,0), 64},
    {CRGB(0,196,0), 255},
    {CRGB(0,64,152), 128}
};
uint8_t currentColorPresetIndex = 0;

RGBW nightlightColor = {CRGB(0,0,0),192};


void setup() {
    pinMode(BTN1PIN, INPUT);
    pinMode(BTN2PIN, INPUT);
    pinMode(BTN3PIN, INPUT);

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

    // initial brightness
    FastLED.setBrightness(pgm_read_byte(&gammaVals[brightnessPresets[currentBrightnessPresetIndex]]));

    // initial color
    fillSolid_RGBW(colorPresets[currentColorPresetIndex]);
    showAllRGBW();

    


    // fill nightlight color into back array
    for (uint8_t i = 0; i<NUMLEDS; i++) {
        previous_leds[i] = nightlightColor.rgb;
    }
    for (uint8_t i = 0; i < NUMWHITE; i++) {
        previous_whiteleds[i] = nightlightColor.w;
    }
}

void loop() {
    FastLED.delay(5);
    showAllRGBW();

    // poll btns
    if ((millis() - btn_previousTickTime) >= BTN_TICK) {
        btn_previousTickTime = millis();

        // poll btn 1
        bool btn1_pressed = pollBtn(BTN1PIN, &btn1_counter);
        if (btn1_pressed) {
            advanceToNextBrightnessPreset();
        }

        // poll btn 2
        bool btn2_pressed = pollBtn(BTN2PIN, &btn2_counter);
        if (btn2_pressed) {
            if (globalMode == GLOBALMODE_NORMAL) {
                advanceToNextColorPreset();
            }
        }

        // poll btn 3
        bool btn3_pressed = pollBtn(BTN3PIN, &btn3_counter);
        if (btn3_pressed) {
            // action on btn 3 press
            toggleNightlightMode();
        }
    }
}

void setRGBWPixel(uint8_t i, RGBW color) {
    if (i < NUMLEDS) {
        leds[i] = color.rgb;
    }
    if (i < NUMWHITE) {
        whiteleds[i] = color.w;
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
    FastLED.show();
    whiteLedsShow();
}

void fillSolid_RGBW(RGBW color) {
    for (uint8_t i = 0; i < NUMLEDS; i++) {
        leds[i] = color.rgb;
    }
    for (uint8_t i = 0; i < NUMWHITE; i++) {
        whiteleds[i] = color.w;
    }
}

// increment btn counter every time we read it as active (LOW);
// reset to 0 every time we read as inactive;
// assume finished bouncing when counter is MAX_VAL (btn has been active for that long)
bool pollBtn(uint8_t btn_pin, uint8_t *btn_counter) {
    uint8_t btn_value = digitalRead(btn_pin);
    if (btn_value == LOW) {
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
    uint8_t newOutputBrightness = brightnessPresets[currentBrightnessPresetIndex];
    FastLED.setBrightness(pgm_read_byte(&gammaVals[newOutputBrightness]));
    FastLED.show();
}

void advanceToNextColorPreset() {
    currentColorPresetIndex++;
    if (currentColorPresetIndex >= NUM_COLOR_PRESETS) {
        currentColorPresetIndex = 0;
    }
    fillSolid_RGBW(colorPresets[currentColorPresetIndex]);
    showAllRGBW();
}

void toggleNightlightMode() {
    if (globalMode == GLOBALMODE_NORMAL) {
        globalMode = GLOBALMODE_NIGHTLIGHT;
    } else {
        globalMode = GLOBALMODE_NORMAL;
    }

    // swap front and back arrays
    for (uint8_t i = 0; i < NUMLEDS; i++) {
        CRGB tmp = previous_leds[i];
        previous_leds[i] = leds[i];
        leds[i] = tmp;
    }
    for (uint8_t i = 0; i<NUMWHITE; i++) {
        uint8_t tmp = previous_whiteleds[i];
        previous_whiteleds[i] = whiteleds[i];
        whiteleds[i] = tmp;
    }

    showAllRGBW();
}

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