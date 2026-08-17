#include "TimingHAL.h"

volatile uint32_t _bareMetalHalMillisCounter = 0;

ISR(TIMER0_COMPA_vect) {
  _bareMetalHalMillisCounter++;
}
