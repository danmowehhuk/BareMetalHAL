#ifndef BAREMETALHAL_AVR_FLASHHAL_H
#define BAREMETALHAL_AVR_FLASHHAL_H

#include <avr/pgmspace.h>

// Non-obvious facts, verified rather than assumed:
// - FlashStr is a project-owned name instead of Arduino's own
//   __FlashStringHelper because that name is reserved (leading
//   double-underscore).
// - readByte() is the one piece that's genuinely target-specific: AVR
//   needs pgm_read_byte (a dedicated instruction, lpm) because flash and
//   RAM are separate address spaces there. ARM/ESP32 don't have that
//   split - verified against the Renesas Uno R4's own pgmspace shim,
//   which defines pgm_read_byte as a plain dereference and PROGMEM/PSTR
//   as no-ops. Adding HAL_ESP32/HAL_ARM means an #elif branch here, not a
//   rewrite - consuming code only ever calls BareMetalHAL::readByte().

namespace BareMetalHAL {

class FlashStr;
inline char readByte(const char* flashPtr) { return (char)pgm_read_byte(flashPtr); }

}  // namespace BareMetalHAL

#define F(string_literal) (reinterpret_cast<const BareMetalHAL::FlashStr*>(PSTR(string_literal)))

#endif
