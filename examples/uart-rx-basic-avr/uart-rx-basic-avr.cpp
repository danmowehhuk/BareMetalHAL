// Bare-metal AVR example proving BareMetalHAL::UartHAL's RX path
// (available()/read()) actually receives bytes, not just compiles.
//
// Requires a physical jumper wire on real hardware (and a matching
// Connector in the SimulIDE circuit) from Uart3's TXD3 pin to Uart2's
// RXD2 pin - Uart3 sends a fixed test string, Uart2 receives it via
// its interrupt-driven ring buffer, and Uart0 (separately, untouched
// by the loopback wiring) reports pass/fail over the existing
// USB-serial connection. Deliberately does NOT use Uart0 for the test
// payload itself - Uart0 only ever needs its normal, already-present
// USB-UART connection, so nothing extra ever needs to be tapped onto
// that shared header.
//
// Expected serial output (9600 baud, on Uart0), repeating roughly
// every second:
//   RX: OK "6BUI-TEST"     (all 9 bytes round-tripped correctly)
//   or
//   RX: FAIL got N of 9 bytes, no byte mismatch (just short)
//     (every byte that did arrive was correct - just too few of them)
//   or
//   RX: FAIL got N of 9 bytes, first mismatch at i=X: expected=Y actual=Z
//     (a byte that arrived didn't match what was sent - real corruption)

#include <util/delay.h>
#include "BareMetalHAL.h"

using namespace BareMetalHAL;

static const char TEST_MSG[] = "6BUI-TEST";
static const uint8_t TEST_LEN = sizeof(TEST_MSG) - 1;  // exclude the trailing '\0'

int main() {
  Uart0::begin(9600);
  Uart2::begin(9600);
  Uart2::enableRx();
  Uart3::begin(9600);
  Uart3::enableRx();

  while (true) {
    for (uint8_t i = 0; i < TEST_LEN; i++) {
      Uart3::write((uint8_t)TEST_MSG[i]);
    }

    _delay_ms(50);  // give the ISR time to drain Uart3's transmission into Uart2's ring buffer

    uint8_t received = 0;
    uint8_t firstMismatchIndex = 0xFF;
    char actualByte = 0;
    while (Uart2::available() > 0 && received < TEST_LEN) {
      int b = Uart2::read();
      if ((char)b != TEST_MSG[received] && firstMismatchIndex == 0xFF) {
        firstMismatchIndex = received;
        actualByte = (char)b;
      }
      received++;
    }

    if (received == TEST_LEN && firstMismatchIndex == 0xFF) {
      Uart0::println("RX: OK \"6BUI-TEST\"");
    } else if (firstMismatchIndex == 0xFF) {
      // Every byte that arrived matched - this is a short count, not corruption.
      Uart0::print("RX: FAIL got ");
      Uart0::print((int)received);
      Uart0::print(" of ");
      Uart0::print((int)TEST_LEN);
      Uart0::println(" bytes, no byte mismatch (just short)");
    } else {
      Uart0::print("RX: FAIL got ");
      Uart0::print((int)received);
      Uart0::print(" of ");
      Uart0::print((int)TEST_LEN);
      Uart0::print(" bytes, first mismatch at i=");
      Uart0::print((int)firstMismatchIndex);
      Uart0::print(": expected=");
      Uart0::print((int)TEST_MSG[firstMismatchIndex]);
      Uart0::print(" actual=");
      Uart0::println((int)actualByte);
    }

    _delay_ms(950);
  }
}
