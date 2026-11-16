// Calls timingInit() but never millis() or delay(), so nothing else
// references TimingHAL.cpp. Its Timer0 ISR must still be linked: if
// it isn't, the first tick jumps to __bad_interrupt and the board
// resets every millisecond.
//
// build.sh links BareMetalHAL as a static archive, the way consumers
// do, and fails if the ISR is missing from the .elf.
//
// Expected serial output (9600 baud): "boot" once, then "alive" every
// second. A reset loop prints only the start of "boot", over and over.

#include <util/delay.h>
#include "BareMetalHAL.h"

using namespace BareMetalHAL;

int main() {
  Uart0::begin(9600);
  Uart0::println("boot");
  timingInit();

  while (true) {
    _delay_ms(1000);
    Uart0::println("alive");
  }
}
