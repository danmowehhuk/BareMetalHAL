// Bare-metal AVR example proving BareMetalHAL::TimingHAL actually
// ticks at the right rate, not just compiles.
//
// Cross-checks Timer0-based millis() against _delay_ms(), an
// independent, F_CPU-calibrated NOP-loop delay that doesn't go through
// Timer0 at all - if millis()'s tick rate were wrong (bad
// prescaler/OCR0A), the reported delta would visibly differ from the
// requested 1000ms delay, whereas checking millis() only for monotonic
// increase couldn't catch a wrong tick rate at all.
//
// Expected serial output (9600 baud), repeating roughly every second:
//   delta=1000ms  (or close to it - SimulIDE's own timing has some
//   tolerance)

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
  }
}
