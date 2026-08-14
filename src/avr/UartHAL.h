#ifndef BAREMETALHAL_AVR_UARTHAL_H
#define BAREMETALHAL_AVR_UARTHAL_H

#if !defined(HAL_AVR)
#error "BareMetalHAL/avr/UartHAL.h: no non-Arduino target selected (define HAL_AVR)."
#endif

#include <stdint.h>
#include <avr/io.h>

// One namespace per physical UART (mirroring Arduino's Serial/Serial1/
// Serial2/Serial3) rather than a runtime-selected index - which UART
// you're wired to is always a compile-time fact on real hardware, never a
// runtime choice. Only Uart0 is implemented so far (the only consumer
// today is TestTool, hardcoded to USART0 in its old per-library HAL
// experiment); Uart1-3 get added here, not touched anywhere else, when
// something actually needs a second UART.
//
// begin()/write() are caller-owned: nothing here self-initializes on
// first use. A lazily-self-initializing UART is a real collision hazard
// the moment more than one library shares it in the same program - the
// sketch's own setup()/main() calls Uart0::begin() exactly once, the same
// job Serial.begin() does on Arduino.
//
// begin()'s baud-rate register math matches Arduino's own
// HardwareSerial::begin() (cores/arduino/HardwareSerial.cpp) - U2X mode
// first, falling back to normal mode if the resulting UBRR would overflow
// its useful range - rather than a hand-derived formula, since getting
// this wrong breaks real serial communication silently.
namespace BareMetalHAL {
namespace Uart0 {

inline void begin(uint32_t baud) {
  uint16_t baudSetting = (uint16_t)((F_CPU / 4 / baud - 1) / 2);
  UCSR0A = (1 << U2X0);
  if (baudSetting > 4095) {
    UCSR0A = 0;
    baudSetting = (uint16_t)((F_CPU / 8 / baud - 1) / 2);
  }
  UBRR0H = (uint8_t)(baudSetting >> 8);
  UBRR0L = (uint8_t)baudSetting;
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);  // 8N1
  UCSR0B = (1 << RXEN0) | (1 << TXEN0);
}

inline void write(uint8_t b) {
  while (!(UCSR0A & (1 << UDRE0))) { }
  UDR0 = b;
}

}  // namespace Uart0
}  // namespace BareMetalHAL

#endif
