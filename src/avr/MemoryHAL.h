#ifndef BAREMETALHAL_AVR_MEMORYHAL_H
#define BAREMETALHAL_AVR_MEMORYHAL_H

#include <stdlib.h>

// Declared at file scope deliberately, not inside the function body below:
// a block-scope `extern` declared inside a function nested in a namespace
// binds to that namespace (BareMetalHAL::__heap_start), not the real
// global symbol avr-libc's linker script defines - verified empirically,
// not assumed. Declaring it here, outside any namespace, means the
// unqualified name inside freeMemory() below resolves via ordinary
// enclosing-scope lookup to the actual global.
extern char __heap_start, *__brkval;

namespace BareMetalHAL {

// Reads AVR-libc's heap high-water-mark globals directly - the same
// technique Arduino's own freeMemory()-style helpers use, since these are
// avr-libc internals, not Arduino's. Valid on any AVR build.
inline int freeMemory() {
  char top;
  if (__brkval == 0) {
    return &top - &__heap_start;
  } else {
    return &top - __brkval;
  }
}

}  // namespace BareMetalHAL

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

inline void* operator new(size_t size) {
  // Even a zero-sized allocation should return a unique pointer, but
  // malloc doesn't guarantee this - same handling as Arduino's own
  // new_helper().
  if (size == 0) size = 1;
  return malloc(size);
}

inline void* operator new[](size_t size) {
  return operator new(size);
}

inline void operator delete(void* ptr) noexcept {
  free(ptr);
}

inline void operator delete[](void* ptr) noexcept {
  operator delete(ptr);
}

#endif
