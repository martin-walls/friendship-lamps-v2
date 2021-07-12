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
#define BTN_MAX_BOUNCE 20 // 20 ms 
uint8_t btn1_counter = 0;


#define LED_PWM_FREQUENCY 5000 /* Hz */
#define LED_PWM_RESOLUTION 8   /* pwm resolution in bits */

extern const uint8_t gammaVals[];

CRGB leds[NUMLEDS];
uint8_t whiteleds[NUMWHITE];

#define NUM_BRIGHTNESS_PRESETS 4
uint8_t brightnessPresets[] = {64, 128, 192, 255};
uint8_t currentBrightnessPresetIndex = 0;

uint8_t testTwinkleArray[NUMWHITE];
int8_t testTwinkleArray2[NUMWHITE];


void setup() {
    pinMode(BTN1PIN, INPUT);
    pinMode(BTN2PIN, INPUT);
    pinMode(BTN3PIN, INPUT);

    FastLED.addLeds<LEDTYPE, LEDPIN, GRB>(leds, NUMLEDS);
    FastLED.setDither(0);

    FastLED.setBrightness(pgm_read_byte(&gammaVals[brightnessPresets[currentBrightnessPresetIndex]]));

    leds[0] = CRGB(255, 0, 0);
    leds[1] = CRGB(0, 255, 0);
    FastLED.show();

    // set up a separate PWM channel for each white LED
    for (int ledIndex = 0; ledIndex < NUMWHITE; ledIndex++) {
        // configure PWM channel for LED i
        ledcSetup(ledIndex /* PWM channel number */, LED_PWM_FREQUENCY, LED_PWM_RESOLUTION);
        // attach the PWM channel to the LED pin
        ledcAttachPin(WHITEPINS[ledIndex] /* pin number */, ledIndex /* PWM channel number */);
    }

    for (int ledIndex = 0; ledIndex < NUMWHITE; ledIndex++) {
        testTwinkleArray[ledIndex] = esp_random(); // array is uint8_t so this is truncated to 0-255
        testTwinkleArray2[ledIndex] = 1;
    }


    whiteleds[0] = 0;
}

void loop() {
    // FastLED.delay(1000);
    // advanceToNextBrightnessPreset();
    // ledcWrite(0, pgm_read_byte(&gammaVals[whiteLedBrightness]));
    // whiteLedBrightness += whiteLedFadeDirection;
    // if (whiteLedBrightness == 255 || whiteLedBrightness == 0) {
    //     whiteLedFadeDirection *= -1;
    // }
    FastLED.delay(5);

    // white leds twinkle
    // for (int i = 0; i<NUMWHITE; i++) {
    //     if (testTwinkleArray[i] == 255) {
    //         testTwinkleArray2[i] = -1;
    //     } else if (testTwinkleArray[i] == 0) {
    //         testTwinkleArray2[i] = random(100) < 5 ? 1 : 0;
    //     }
    //     testTwinkleArray[i] += testTwinkleArray2[i];
    //     whiteleds[i] = pgm_read_byte(&gammaVals[testTwinkleArray[i]]);
    // }
    // whiteLedsShow();


    // if (digitalRead(BTN1PIN) == 0) {
    //     FastLED.delay(20);
    //     if (digitalRead(BTN1PIN) == 0) {
    //         advanceToNextBrightnessPreset();
    //         while (digitalRead(BTN1PIN) == 0) {}
    //     }
    // }

    // increment btn counter every time we read it as active (LOW); 
    // reset to 0 every time we read as inactive;
    // assume finished bouncing when counter is MAX_VAL (btn has been active for that long)
    if ((millis() - btn_previousTickTime) >= BTN_TICK) {
        btn_previousTickTime = millis();
        uint8_t btn1_value = digitalRead(BTN1PIN);
        if (btn1_value == 0) {
            btn1_counter++;
        } else {
            btn1_counter = 0;
        }
        
        if (btn1_counter == BTN_MAX_BOUNCE) {
            advanceToNextBrightnessPreset();
        }
    }
    



}

void whiteLedsShow() {
    for (int i = 0; i < NUMWHITE; i++) {
        // read value for this LED from array
        // and update LED output value
        ledcWrite(i, whiteleds[i]);
    }
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