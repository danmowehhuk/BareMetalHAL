// Compile/link smoke test only - see the SD driver work in a sibling
// repo for real verification.
#include <BareMetalHAL.h>

using namespace BareMetalHAL;

int main() {
  spiBegin();
  uint8_t echoed = spiTransfer(0xFF);
  (void)echoed;
  while (true) {}
  return 0;
}
