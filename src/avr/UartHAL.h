#ifndef BAREMETALHAL_AVR_UARTHAL_H
#define BAREMETALHAL_AVR_UARTHAL_H

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "FlashHAL.h"

// Non-obvious facts, verified against avr-libc rather than assumed:
// - USART3's registers sit at a non-contiguous address from 0/1/2 (0x130
//   vs. 0xC0/0xC8/0xD0) - avr/iomxx0_1.h. <avr/io.h> resolves the correct
//   per-chip header via -mmcu, so no per-chip logic is needed here.
// - Bit positions (U2X, UDRE, RXEN, TXEN, UCSZ1/0) ARE identical across
//   every UART instance - avr/iomxx0_1.h - so detail::uartBegin/uartWrite
//   below hardcode them via UART0's names despite being generic across
//   all 4 UARTs.
// - Smaller AVR chips (e.g. the 328P) don't define UART1-3's registers at
//   all - avr/iom328p.h - hence the #ifdef UDR1/2/3 guards below; without
//   them, this header would fail to compile on such a chip.
// - begin()/write() are caller-owned - nothing here self-initializes.
//   Two libraries lazily self-initializing the same UART is a real
//   collision hazard, so the sketch's own setup()/main() calls begin()
//   exactly once, same as Serial.begin() on Arduino.
// - begin()'s baud-rate math matches Arduino's HardwareSerial::begin(),
//   not a hand-derived formula.
// - print()/println()'s Write parameter is a non-type template parameter,
//   not a runtime function pointer - the compiler resolves and inlines it
//   at compile time.
namespace BareMetalHAL {

namespace detail {

inline void uartBegin(volatile uint8_t& ucsra, volatile uint8_t& ucsrb,
                       volatile uint8_t& ucsrc, volatile uint8_t& ubrrh,
                       volatile uint8_t& ubrrl, uint32_t baud) {
  uint16_t baudSetting = (uint16_t)((F_CPU / 4 / baud - 1) / 2);
  ucsra = (1 << U2X0);
  if (baudSetting > 4095) {
    ucsra = 0;
    baudSetting = (uint16_t)((F_CPU / 8 / baud - 1) / 2);
  }
  ubrrh = (uint8_t)(baudSetting >> 8);
  ubrrl = (uint8_t)baudSetting;
  ucsrc = (1 << UCSZ01) | (1 << UCSZ00);  // 8N1
  ucsrb = (1 << RXEN0) | (1 << TXEN0);
}

inline void uartWrite(volatile uint8_t& ucsra, volatile uint8_t& udr, uint8_t b) {
  while (!(ucsra & (1 << UDRE0))) { }
  udr = b;
}

template <void (*Write)(uint8_t)>
inline void print(const char* s) { while (s && *s) Write((uint8_t)*s++); }

template <void (*Write)(uint8_t)>
inline void print(char c) { Write((uint8_t)c); }

template <void (*Write)(uint8_t)>
inline void print(int v) {
  unsigned int uv;
  if (v < 0) {
    Write((uint8_t)'-');
    uv = (unsigned int)(-(long)v);
  } else {
    uv = (unsigned int)v;
  }
  // sizeof(unsigned int)*3 covers the max decimal digit count for any int
  // width - not hardcoded for AVR's 16-bit int, since this also runs
  // under a future 32-bit-int backend.
  char digits[sizeof(unsigned int) * 3];
  uint8_t n = 0;
  do {
    digits[n++] = '0' + (uv % 10);
    uv /= 10;
  } while (uv > 0);
  while (n > 0) Write((uint8_t)digits[--n]);
}

template <void (*Write)(uint8_t)>
inline void print(const FlashStr* s) {
  const char* p = reinterpret_cast<const char*>(s);
  char c;
  while ((c = readByte(p++)) != '\0') Write((uint8_t)c);
}

template <void (*Write)(uint8_t)>
inline void println(const char* s) { print<Write>(s); print<Write>('\r'); print<Write>('\n'); }

template <void (*Write)(uint8_t)>
inline void println(int v) { print<Write>(v); print<Write>('\r'); print<Write>('\n'); }

template <void (*Write)(uint8_t)>
inline void println(const FlashStr* s) { print<Write>(s); print<Write>('\r'); print<Write>('\n'); }

}  // namespace detail

// Adding a 5th UART: one more invocation below, gated the same way.
#define BAREMETALHAL_DEFINE_UART(N) \
namespace Uart##N { \
namespace rxDetail { \
  extern volatile uint8_t buffer[64]; \
  extern volatile uint8_t head; \
  extern volatile uint8_t tail; \
} \
inline void begin(uint32_t baud) { \
  detail::uartBegin(UCSR##N##A, UCSR##N##B, UCSR##N##C, UBRR##N##H, UBRR##N##L, baud); \
} \
inline void write(uint8_t b) { detail::uartWrite(UCSR##N##A, UDR##N, b); } \
void enableRx(); \
inline void print(const char* s) { detail::print<write>(s); } \
inline void print(char c) { detail::print<write>(c); } \
inline void print(int v) { detail::print<write>(v); } \
inline void print(const FlashStr* s) { detail::print<write>(s); } \
inline void println(const char* s) { detail::println<write>(s); } \
inline void println(int v) { detail::println<write>(v); } \
inline void println(const FlashStr* s) { detail::println<write>(s); } \
inline uint8_t available() { \
  return (uint8_t)(rxDetail::tail - rxDetail::head) & 63; \
} \
inline int read() { \
  if (rxDetail::head == rxDetail::tail) return -1; \
  uint8_t b = rxDetail::buffer[rxDetail::head]; \
  rxDetail::head = (rxDetail::head + 1) & 63; \
  return b; \
} \
}

BAREMETALHAL_DEFINE_UART(0)

#ifdef UDR1
BAREMETALHAL_DEFINE_UART(1)
#endif

#ifdef UDR2
BAREMETALHAL_DEFINE_UART(2)
#endif

#ifdef UDR3
BAREMETALHAL_DEFINE_UART(3)
#endif

#undef BAREMETALHAL_DEFINE_UART

}  // namespace BareMetalHAL

#endif
