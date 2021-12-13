#include "buttons.h"

namespace Btns {

  /* Button functions for general use */
  Btn brightnessBtn{PIN_FRONT};
  Btn colorBtn{PIN_SIDE_THREE_FRONT};
  Btn nightlightBtn{PIN_SIDE_SINGLE};
  Btn effectBtn{PIN_SIDE_THREE_MIDDLE};
  Btn morsecodeBtn{PIN_SIDE_THREE_BACK};

  /* Button functions on startup */
  Btn wifiDisableBtn{PIN_FRONT};
  Btn wifiResetBtn{PIN_SIDE_SINGLE};

  uint32_t previousTickTime = 0;

  /*
   * Configure button inputs.
   * To be run in setup().
   */
  void setupBtns() {
    pinMode(PIN_FRONT, INPUT);
    pinMode(PIN_SIDE_SINGLE,       INPUT);
    pinMode(PIN_SIDE_THREE_FRONT,  INPUT);
    pinMode(PIN_SIDE_THREE_MIDDLE, INPUT);
    pinMode(PIN_SIDE_THREE_BACK,   INPUT);
  }

  /*
   * Return whether the given button is currently pressed.
   */
  bool isBtnPressed(Btn& btn) {
    return digitalRead(btn.pin) == LOW;
  }

  /*
   * Blocks until the specified button is no longer pressed.
   */
  void waitForBtnRelease(Btn& btn) {
    while (isBtnPressed(btn)) {
      delay(10);
    }
  }

  /*
   * For non-blocking button debouncing.
   *
   * Increment the counter for this button every time we read it as
   * `value`. Every time we read it as not `value`, reset the counter
   * to 0.
   */
  bool pollBtnForValue(Btn& btn, uint8_t value) {
    uint8_t btnValue = digitalRead(btn.pin);
    if (btnValue == PRESSED) {
      btn.counter_on++;
      btn.counter_off = 0;
    } else {
      btn.counter_on = 0;
      btn.counter_off++;
    }

    if (value == PRESSED) {
      return btn.counter_on == MAX_BOUNCE;
    } else {
      return btn.counter_off == MAX_BOUNCE;
    }
  }

  bool pollBtn(Btn& btn) {
    pollBtnForValue(btn, PRESSED);
  }

}
