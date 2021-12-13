#ifndef BUTTONS_H_
#define BUTTONS_H_

#include <stdint.h>
#include <Arduino.h>

namespace Btns {

  class Btn {
    private:
      // forbid copying class
      Btn(const Btn& b);
      Btn& operator=(const Btn& b);

    public:
      const uint8_t pin;
      uint16_t counter_on = 0;
      uint16_t counter_off = 0;

      Btn(uint8_t p) : pin(p) {}
  };

  /* Button is pressed when we read LOW */
  const uint8_t PRESSED = LOW;
  const uint8_t UNPRESSED = HIGH;

  /* Pin definitions */
  const uint8_t PIN_FRONT             = 10;
  const uint8_t PIN_SIDE_SINGLE       = 9;
  const uint8_t PIN_SIDE_THREE_FRONT  = 5;
  const uint8_t PIN_SIDE_THREE_MIDDLE = 18;
  const uint8_t PIN_SIDE_THREE_BACK   = 23;

  /* Tick time between reading button when debouncing. (ms) */
  const uint8_t TICK = 1;

  /* The number of ticks we need to read a button for before
   * it's debounced.
   */
  const uint8_t MAX_BOUNCE = 15;

  extern uint32_t previousTickTime;


  /* Button functions for general use */
  extern Btn brightnessBtn;
  extern Btn colorBtn;
  extern Btn nightlightBtn;
  extern Btn effectBtn;
  extern Btn morsecodeBtn;

  /* Button functions on startup */
  extern Btn wifiDisableBtn;
  extern Btn wifiResetBtn;

  /*
   * Configure button inputs. To be run in setup().
   */
  void setupBtns();

  /*
   * Return whether the given button is currently pressed.
   */
  bool isBtnPressed(Btn& btn);

  /*
   * Blocks until the specified button is no longer pressed.
   */
  void waitForBtnRelease(Btn& btn);

  /*
   * For non-blocking button debouncing.
   *
   * Increment the counter for this button every time we read it as
   * `value`. Every time we read it as not `value`, reset the counter
   * to 0.
   */
  bool pollBtnForValue(Btn& btn, uint8_t value=PRESSED);

  bool pollBtn(Btn& btn);
}


#endif
