#include "gamma.h"

// apply gamma correction to value
uint8_t Gamma::gammaCorrect(uint8_t value) {
  return pgm_read_byte(&Gamma::gammaVals[value]);
}
