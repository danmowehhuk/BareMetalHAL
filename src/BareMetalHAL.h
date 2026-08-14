#ifndef BAREMETALHAL_H
#define BAREMETALHAL_H

#ifndef NO_ARDUINO
#error "BareMetalHAL is only needed off Arduino - gate #include <BareMetalHAL.h> behind #if defined(NO_ARDUINO) in your own code, don't include it unconditionally."
#endif

// Selects one platform folder based on which HAL_xxx flag is defined -
// the only place in this library (and the only place any consuming
// library ever needs to look) that enumerates target flags by name. Each
// platform folder implements the same interface, documented in
// README.md; adding a new platform means adding a new branch here and a
// new folder, never touching an existing platform's files.
#if defined(HAL_AVR)
#include "avr/FlashHAL.h"
#include "avr/UartHAL.h"
#include "avr/MemoryHAL.h"
#else
#error "BareMetalHAL: no supported HAL_xxx target defined. Supported: HAL_AVR."
#endif

#endif
