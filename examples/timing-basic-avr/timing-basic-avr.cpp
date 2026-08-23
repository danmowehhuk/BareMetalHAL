// Bare-metal AVR example proving BareMetalHAL::TimingHAL actually
// ticks at the right rate, not just compiles.
//
// Cross-checks Timer0-based millis() against _delay_ms(), an
// independent, F_CPU-calibrated NOP-loop delay that doesn't go through
// Timer0 at all - if millis()'s tick rate were wrong (bad
// prescaler/OCR0A), the reported delta would visibly differ from the
// requested 1000ms delay, whereas checking millis() only for monotonic
// increase couldn't catch a wrong tick rate at all.
// Also cross-checks delay() the same way, against the same millis()
// ground truth. Finally cross-checks delayMicroseconds() by aggregating
// 1000 x 1000us calls and comparing the total elapsed time to millis().
//
// Expected serial output (9600 baud), repeating roughly every second:
//   delta=1000ms  (or close to it - SimulIDE's own timing has some
//   tolerance)
//   delay_delta=1000ms  (or close to it, same tolerance)
//   us_delta=1000ms  (or close to it, same tolerance)

#include <util/delay.h>
#include "BareMetalHAL.h"

using namespace BareMetalHAL;

int main() {
  Uart0::begin(9600);
  timingInit();

  while (true) {
    uint32_t start = millis();
    _delay_ms(1000);
    uint32_t delta = millis() - start;

    Uart0::print("delta=");
    Uart0::print((int)delta);
    Uart0::println("ms");

    uint32_t delayStart = millis();
    delay(1000);
    uint32_t delayDelta = millis() - delayStart;

    Uart0::print("delay_delta=");
    Uart0::print((int)delayDelta);
    Uart0::println("ms");

    uint32_t usStart = millis();
    for (uint16_t i = 0; i < 1000; i++) {
      delayMicroseconds(1000);
    }
    uint32_t usDelta = millis() - usStart;

    Uart0::print("us_delta=");
    Uart0::print((int)usDelta);
    Uart0::println("ms");
  }
}
