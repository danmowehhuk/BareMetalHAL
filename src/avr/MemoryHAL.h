#ifndef BAREMETALHAL_AVR_MEMORYHAL_H
#define BAREMETALHAL_AVR_MEMORYHAL_H

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

#endif
