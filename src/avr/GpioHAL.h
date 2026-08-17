#ifndef BAREMETALHAL_AVR_GPIOHAL_H
#define BAREMETALHAL_AVR_GPIOHAL_H

#include <stdint.h>

// Packs a port + bit into one uint8_t so it can pass through consuming
// libraries' existing runtime callback function pointers (e.g.
// Eventuino's pinSetupCallback_t = void(*)(uint8_t)) unchanged - the
// callback signature can't carry a compile-time template parameter the
// way BareMetalHAL::UartHAL does, since the whole point of these
// callbacks is runtime substitutability. Bits 3-7 = port index, bits
// 0-2 = bit index (0-7; AVR ports are 8 bits wide). Only port indices
// 0-10 (Port::A - Port::L) are ever produced by pin() below; indices
// 11-31 are unreachable through this factory and, not coincidentally,
// also unhandled by GpioHAL's register-decode switch (see the facade
// functions further down this file) - an out-of-range encoding falls
// through to that same switch's default no-op path with no separate
// sentinel value needed.
namespace BareMetalHAL {

// ATmega2560's 11 usable ports. AVR skips "I" entirely (no PORTI on any
// AVR chip - verified against avr-libc's iomxx0_1.h, not assumed).
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

#endif
