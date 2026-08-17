#ifndef BAREMETALHAL_AVR_GPIOHAL_H
#define BAREMETALHAL_AVR_GPIOHAL_H

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

// Packs a port + bit into one uint8_t so it can pass through consuming
// libraries' existing runtime callback function pointers (e.g.
// Eventuino's pinSetupCallback_t = void(*)(uint8_t)) unchanged. 
// Bits 3-7 = port index, bits 0-2 = bit index (0-7; AVR ports are 8 
// bits wide).
namespace BareMetalHAL {

// The AVR family's 11 usable ports (no `PORTI` on any AVR chip -
// verified against avr-libc's iomxx0_1.h, not assumed).
enum class Port : uint8_t { A, B, C, D, E, F, G, H, J, K, L };

constexpr uint8_t pin(Port port, uint8_t bit) {
  return (static_cast<uint8_t>(port) << 3) | (bit & 0x7);
}

}  // namespace BareMetalHAL

// Compile-time checks of the packing math itself - not part of the
// public API, just proof the bit arithmetic above is correct before
// anything else in this file (or a consuming library) depends on it.
static_assert(BareMetalHAL::pin(BareMetalHAL::Port::A, 0) == 0x00,
              "Port::A bit 0 must pack to 0x00");
static_assert(BareMetalHAL::pin(BareMetalHAL::Port::D, 3) == 0x1B,
              "Port::D bit 3 must pack to 0x1B (3<<3 | 3)");
static_assert(BareMetalHAL::pin(BareMetalHAL::Port::L, 7) == 0x57,
              "Port::L bit 7 must pack to 0x57 (10<<3 | 7)");

namespace BareMetalHAL {

namespace detail {

// Single decode point shared by pinMode/digitalWrite/digitalRead below,
// rather than three separate switches - one register lookup covers all
// three registers a caller might need (DDRx for direction, PORTx for
// output level or pull-up enable, PINx for input read). Returns false
// for a port index with no matching case, which includes both
// genuinely out-of-range values (11-31) and any port this specific chip
// doesn't have (the #ifdef guards below skip a case entirely if this
// chip's <avr/io.h> didn't define that port's registers, mirroring
// UartHAL.h's #ifdef UDR1/2/3 guards for the same reason).
inline bool resolvePort(uint8_t portIndex, volatile uint8_t*& ddr,
                         volatile uint8_t*& port, volatile uint8_t*& pinReg) {
  switch (portIndex) {
#ifdef DDRA
    case 0: ddr = &DDRA; port = &PORTA; pinReg = &PINA; return true;
#endif
#ifdef DDRB
    case 1: ddr = &DDRB; port = &PORTB; pinReg = &PINB; return true;
#endif
#ifdef DDRC
    case 2: ddr = &DDRC; port = &PORTC; pinReg = &PINC; return true;
#endif
#ifdef DDRD
    case 3: ddr = &DDRD; port = &PORTD; pinReg = &PIND; return true;
#endif
#ifdef DDRE
    case 4: ddr = &DDRE; port = &PORTE; pinReg = &PINE; return true;
#endif
#ifdef DDRF
    case 5: ddr = &DDRF; port = &PORTF; pinReg = &PINF; return true;
#endif
#ifdef DDRG
    case 6: ddr = &DDRG; port = &PORTG; pinReg = &PING; return true;
#endif
#ifdef DDRH
    case 7: ddr = &DDRH; port = &PORTH; pinReg = &PINH; return true;
#endif
#ifdef DDRJ
    case 8: ddr = &DDRJ; port = &PORTJ; pinReg = &PINJ; return true;
#endif
#ifdef DDRK
    case 9: ddr = &DDRK; port = &PORTK; pinReg = &PINK; return true;
#endif
#ifdef DDRL
    case 10: ddr = &DDRL; port = &PORTL; pinReg = &PINL; return true;
#endif
    default:
      return false;
  }
}

}  // namespace detail

// Values match Arduino's own (verified against the Arduino AVR core's
// Arduino.h, not assumed) so no translation is needed at any call site
// that happens to pass a literal instead of these names.
constexpr uint8_t INPUT = 0x0;
constexpr uint8_t OUTPUT = 0x1;
constexpr uint8_t INPUT_PULLUP = 0x2;
constexpr uint8_t HIGH = 0x1;
constexpr uint8_t LOW = 0x0;

inline void pinMode(uint8_t packedPin, uint8_t mode) {
  volatile uint8_t *ddr, *port, *pinReg;
  if (!detail::resolvePort(packedPin >> 3, ddr, port, pinReg)) return;
  uint8_t bit = packedPin & 0x7;
  uint8_t oldSREG = SREG;
  cli();
  if (mode == OUTPUT) {
    *ddr |= (1 << bit);
  } else {
    // INPUT and INPUT_PULLUP are both DDR=0 (input); INPUT_PULLUP
    // additionally sets PORTx=1, which on AVR enables the internal
    // pull-up resistor only when the pin is configured as an input -
    // the same DDR+PORT combination Arduino's own pinMode() relies on.
    *ddr &= ~(1 << bit);
    if (mode == INPUT_PULLUP) {
      *port |= (1 << bit);
    } else {
      *port &= ~(1 << bit);
    }
  }
  SREG = oldSREG;
}

inline void digitalWrite(uint8_t packedPin, uint8_t value) {
  volatile uint8_t *ddr, *port, *pinReg;
  if (!detail::resolvePort(packedPin >> 3, ddr, port, pinReg)) return;
  uint8_t bit = packedPin & 0x7;
  uint8_t oldSREG = SREG;
  cli();
  if (value) {
    *port |= (1 << bit);
  } else {
    *port &= ~(1 << bit);
  }
  SREG = oldSREG;
}

inline uint8_t digitalRead(uint8_t packedPin) {
  volatile uint8_t *ddr, *port, *pinReg;
  if (!detail::resolvePort(packedPin >> 3, ddr, port, pinReg)) return LOW;
  uint8_t bit = packedPin & 0x7;
  return (*pinReg & (1 << bit)) ? HIGH : LOW;
}

}  // namespace BareMetalHAL

#endif
