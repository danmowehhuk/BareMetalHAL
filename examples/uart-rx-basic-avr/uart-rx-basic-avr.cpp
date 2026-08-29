// Bare-metal AVR example proving BareMetalHAL::UartHAL's RX path
// (available()/read()) actually receives bytes, not just compiles.
//
// Requires a physical jumper wire on real hardware (and a matching
// Connector in the SimulIDE circuit) from Uart0's TXD0 pin to Uart1's
// RXD1 pin - Uart0 sends a fixed test string, Uart1 receives it via
// its interrupt-driven ring buffer, and Uart0 itself reports pass/fail
// (Uart0's own TX is unaffected by also being wired to Uart1's RX -
// USART TX is push-pull output, driving that net doesn't stop Uart0's
// own serial monitor connection from also reading it, assuming both
// are simulated/wired as inputs listening to the same driven line).
//
// Expected serial output (9600 baud, on Uart0), repeating roughly
// every second:
//   RX: OK "6BUI-TEST"     (all 9 bytes round-tripped correctly)
//   or
//   RX: FAIL got N of 9 bytes, first mismatch at i=X: expected=Y actual=Z

#include <util/delay.h>
#include "BareMetalHAL.h"

using namespace BareMetalHAL;

static const char TEST_MSG[] = "6BUI-TEST";
static const uint8_t TEST_LEN = sizeof(TEST_MSG) - 1;  // exclude the trailing '\0'

int main() {
  Uart0::begin(9600);
  Uart1::begin(9600);

  while (true) {
    for (uint8_t i = 0; i < TEST_LEN; i++) {
      Uart0::write((uint8_t)TEST_MSG[i]);
    }

    _delay_ms(50);  // give the ISR time to drain Uart0's transmission into Uart1's ring buffer

    uint8_t received = 0;
    uint8_t firstMismatchIndex = 0xFF;
    char actualByte = 0;
    while (Uart1::available() > 0 && received < TEST_LEN) {
      int b = Uart1::read();
      if ((char)b != TEST_MSG[received] && firstMismatchIndex == 0xFF) {
        firstMismatchIndex = received;
        actualByte = (char)b;
      }
      received++;
    }

    if (received == TEST_LEN && firstMismatchIndex == 0xFF) {
      Uart0::println("RX: OK \"6BUI-TEST\"");
    } else {
      Uart0::print("RX: FAIL got ");
      Uart0::print((int)received);
      Uart0::print(" of ");
      Uart0::print((int)TEST_LEN);
      Uart0::print(" bytes, first mismatch at i=");
      Uart0::print((int)firstMismatchIndex);
      Uart0::print(": expected=");
      Uart0::print((int)(firstMismatchIndex != 0xFF ? TEST_MSG[firstMismatchIndex] : 0));
      Uart0::print(" actual=");
      Uart0::println((int)actualByte);
    }

    _delay_ms(950);
  }
}
