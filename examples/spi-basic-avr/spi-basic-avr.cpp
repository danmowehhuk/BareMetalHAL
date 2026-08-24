// Compile/link smoke test only - see the SD driver example for real
// verification.
#include <BareMetalHAL.h>

using namespace BareMetalHAL;

int main() {
  // This board's SPI pins (verified against pins_arduino.h): SCK=PB1,
  // MOSI=PB2, MISO=PB3.
  spiBegin(pin(Port::B, 1), pin(Port::B, 2), pin(Port::B, 3));
  uint8_t echoed = spiTransfer(0xFF);
  (void)echoed;
  while (true) {}
  return 0;
}
