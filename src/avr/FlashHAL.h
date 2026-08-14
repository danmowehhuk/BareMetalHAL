#ifndef BAREMETALHAL_AVR_FLASHHAL_H
#define BAREMETALHAL_AVR_FLASHHAL_H

#if defined(HAL_AVR)
#include <avr/pgmspace.h>
#else
#error "BareMetalHAL/avr/FlashHAL.h: no non-Arduino target selected (define HAL_AVR)."
#endif

// FlashStr: a type-tag for "this pointer is to a flash-resident string,"
// used purely for overload resolution - never defined/instantiated, only
// ever seen as FlashStr*. Project-owned name rather than Arduino's
// reserved __FlashStringHelper.
//
// F(x) mirrors Arduino's own trick: PSTR places the literal in flash and
// returns a const char*; the cast is only there so overload resolution can
// tell a flash string apart from a RAM one.
//
// readByte(): the actual flash-read primitive, since this genuinely
// differs by target - AVR needs a dedicated instruction (lpm, via
// pgm_read_byte) because flash and RAM are separate address spaces there.
// ARM/ESP32 have flash memory-mapped into the normal address space, so
// it's just a dereference there - verified against the Renesas Uno R4's
// own pgmspace compatibility shim, which defines pgm_read_byte as exactly
// that (`*(const unsigned char*)(addr)`), and PROGMEM/PSTR as no-ops.
// When HAL_ESP32/HAL_ARM are added, this file grows an #elif branch here,
// not a rewrite - consuming libraries never see the difference, they only
// ever call BareMetalHAL::readByte().

namespace BareMetalHAL {

#if defined(HAL_AVR)
class FlashStr;
inline char readByte(const char* flashPtr) { return (char)pgm_read_byte(flashPtr); }
#endif

}  // namespace BareMetalHAL

#if defined(HAL_AVR)
#define F(string_literal) (reinterpret_cast<const BareMetalHAL::FlashStr*>(PSTR(string_literal)))
#endif

#endif
