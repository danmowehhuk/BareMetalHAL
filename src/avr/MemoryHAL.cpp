#include <stdlib.h>
#include "MemoryHAL.h"

// Global (not namespaced) - operator new/delete overloads only take
// effect for unqualified new/delete expressions when defined at global
// scope, matching Arduino's own cores/arduino/new.cpp (not namespaced
// there either).
//
// Scoped to exactly what a HAL_AVR consumer's new/delete[] expressions
// need (verified via grep across every currently-migrated library:
// only plain `new T[n]` / `delete[]` appears anywhere) - not Arduino's
// fuller new.cpp surface (nothrow overloads, placement new/delete,
// C++14 sized delete). Add one of those here if a future consumer
// actually needs it.
//
// Matches Arduino's default allocation-failure behavior: returns null
// (this toolchain has no exception support to throw from, and
// Arduino's own new.cpp only terminates if NEW_TERMINATES_ON_FAILURE is
// explicitly defined, which it isn't by default).
//
// Deliberately NOT inline, and living in this .cpp rather than
// MemoryHAL.h: at -Os, GCC fully inlines an `inline`-qualified operator
// new/delete at every call site and emits no out-of-line definition in
// any object file, so a translation unit that uses new/delete[] but
// doesn't itself include MemoryHAL.h (e.g. Eventuino.cpp, which only
// includes its own EventuinoHal.h) has nothing to link against -
// confirmed empirically. Same pattern as TimingHAL.cpp needing external
// linkage for its ISR/counter; matches Arduino's own new.cpp, which is
// also a non-inline .cpp definition, not a header.

void* operator new(size_t size) {
  // Even a zero-sized allocation should return a unique pointer, but
  // malloc doesn't guarantee this - same handling as Arduino's own
  // new_helper().
  if (size == 0) size = 1;
  return malloc(size);
}

void* operator new[](size_t size) {
  return operator new(size);
}

void operator delete(void* ptr) noexcept {
  free(ptr);
}

void operator delete[](void* ptr) noexcept {
  operator delete(ptr);
}

// Itanium C++ ABI runtime-support symbols, not Arduino/avr-libc API -
// same class of gap as operator new/delete above (Pitfall 11: nothing
// but Arduino's own core or a hosted libstdc++ supplies these by
// default, and neither exists in a HAL_AVR build). A class with a pure
// virtual method gets a vtable slot pointing at __cxa_pure_virtual
// unconditionally, in whichever translation unit defines that class's
// "key function" (its first out-of-line non-pure virtual, typically the
// destructor) - regardless of whether any subclass actually overrides
// the pure virtual. __cxa_deleted_virtual is the C++11 analog for a
// virtual explicitly marked `= delete`. Mirrors Arduino's own
// cores/arduino/abi.cpp exactly (verified directly against the real
// file, not assumed): both trap via std::terminate() -> abort(), both
// noreturn.
namespace std {
[[gnu::weak, noreturn]] void terminate() {
  abort();
}
}  // namespace std

extern "C" void __cxa_pure_virtual(void) __attribute__ ((__noreturn__));
extern "C" void __cxa_deleted_virtual(void) __attribute__ ((__noreturn__));

extern "C" void __cxa_pure_virtual(void) {
  std::terminate();
}

extern "C" void __cxa_deleted_virtual(void) {
  std::terminate();
}
