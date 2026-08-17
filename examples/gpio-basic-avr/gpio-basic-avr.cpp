// Bare-metal AVR example proving BareMetalHAL::GpioHAL actually decodes
// registers correctly, not just compiles - see README.md's GpioHAL.h
// section for the API this exercises. Deliberately spans three ports
// (L, A, C) rather than one, to catch a copy-paste error in any single
// port's case of the register-decode switch that a single-port test
// would miss.
//
// Wiring (real hardware or SimulIDE): jumper PL7 to PA0. PC3 stays
// unconnected - it's testing the internal pull-up, so an external
// connection would defeat the point.
//
// Expected serial output (9600 baud), repeating every ~1s:
//   PL7=HIGH PA0=HIGH PC3=HIGH
//   PL7=LOW PA0=LOW PC3=HIGH
// PC3 should read HIGH on every line - if it ever reads LOW, the
// INPUT_PULLUP branch of pinMode() is broken. PA0 should always match
// PL7 - if it doesn't, digitalWrite/digitalRead or the PL7/PA0 decode
// cases in detail::resolvePort are broken.

#include <util/delay.h>
#include "BareMetalHAL.h"

using namespace BareMetalHAL;

int main() {
  Uart0::begin(9600);

  pinMode(pin(Port::L, 7), OUTPUT);
  pinMode(pin(Port::A, 0), INPUT);
  pinMode(pin(Port::C, 3), INPUT_PULLUP);

  bool level = true;
  while (true) {
    digitalWrite(pin(Port::L, 7), level ? HIGH : LOW);
    _delay_ms(50);  // let the level settle before reading it back

    Uart0::print("PL7=");
    Uart0::print(level ? "HIGH" : "LOW");
    Uart0::print(" PA0=");
    Uart0::print(digitalRead(pin(Port::A, 0)) == HIGH ? "HIGH" : "LOW");
    Uart0::print(" PC3=");
    Uart0::println(digitalRead(pin(Port::C, 3)) == HIGH ? "HIGH" : "LOW");

    level = !level;
    _delay_ms(950);
  }
}
