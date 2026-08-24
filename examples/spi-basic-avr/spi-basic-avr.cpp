// Compile/link smoke test only - see the SD driver example for real
// verification.
#include <BareMetalHAL.h>

using namespace BareMetalHAL;

int main() {
  // ATmega2560 SPI pins (verified against pins_arduino.h): SCK=PB1,
  // MOSI=PB2, MISO=PB3. A different AVR chip would need its own mapping -
  // SpiHAL itself makes no assumption about which pins these are.
  pinMode(pin(Port::B, 1), OUTPUT); // SCK
  pinMode(pin(Port::B, 2), OUTPUT); // MOSI
  pinMode(pin(Port::B, 3), INPUT);  // MISO

  spiBegin();
  uint8_t echoed = spiTransfer(0xFF);
  (void)echoed;
  while (true) {}
  return 0;
}
