# BareMetalHAL

A shared hardware abstraction layer for libraries in this family (TestTool,
and eventually StreamableDTO, SDStorage, ...) when they're built without
Arduino. One library covers every non-Arduino target - which backend is
active is resolved entirely inside this library, so a consuming library's
own facade never needs to know AVR from ESP32 from ARM, and adding a new
target never means touching a consuming library's code.

## Who needs this installed

Only projects building a consuming library with `-DNO_ARDUINO`. Building
for Arduino needs nothing from this library at all - every consuming
library's own facade falls back to the Arduino API in that case, and this
library is never even included.

## Build flags

A non-Arduino build passes two flags together:

- `-DNO_ARDUINO` - every consuming library's own facade checks this one
  flag directly to decide "Arduino or not." It never enumerates specific
  non-Arduino targets.
- `-DHAL_AVR` (only supported target today; `-DHAL_ESP32`/`-DHAL_ARM`
  later) - tells *this* library which backend to compile in. This is the
  only flag name this library's own code checks by name - no consuming
  library ever does.

Example: `-DNO_ARDUINO -DHAL_AVR`.

## Folder layout

```
src/
  BareMetalHAL.h   - dispatcher: picks a platform folder based on -DHAL_xxx
  avr/             - the HAL_AVR backend
    FlashHAL.h
    UartHAL.h
    MemoryHAL.h
```

Each platform gets its own folder with the same three filenames. The
folder *is* the implementation; `BareMetalHAL.h` only ever routes to one
of them.

## The contract: what every platform folder must provide

This is the complete interface `avr/` implements. A new platform folder
needs to provide the same names with the same signatures and semantics.

**`FlashHAL.h`**
- `namespace BareMetalHAL { class FlashStr; }` - an opaque type-tag for
  "this pointer is to a flash-resident string." Never defined or
  instantiated, only ever seen as `const FlashStr*` - it exists purely so
  overload resolution can tell a flash string apart from a RAM one. On a
  platform where flash and RAM are the same address space (no distinct
  read path needed), this can still exist as an unused tag type - see
  `readByte()` below.
- `#define F(string_literal) (...)` - a macro that marks a string literal
  as flash-resident, expanding to a `const BareMetalHAL::FlashStr*`.
  Mirrors Arduino's own `F()`/`__FlashStringHelper` trick. On a platform
  where flash and RAM are the same address space, this can simply expand
  to `string_literal` unchanged (no cast needed).
- `char readByte(const char* flashPtr)` - reads one byte from a
  flash-resident string. This is the one genuinely target-specific piece:
  AVR needs a dedicated instruction (`pgm_read_byte`) because flash and
  RAM are separate address spaces; a platform with flash memory-mapped
  into the normal address space (verified for ARM via the Renesas Uno
  R4's own pgmspace compatibility shim - likely true for ESP32 too, but
  unverified) can just implement this as `return *flashPtr;`.

**`UartHAL.h`**
- `namespace BareMetalHAL::Uart<n> { void begin(uint32_t baud); void
  write(uint8_t b); }` - one namespace per physical UART, not a
  runtime-selected index (which UART you're wired to is a compile-time
  fact on real hardware, never a runtime choice). The `avr/` backend
  implements `Uart0`-`Uart3` (the ATmega2560's 4 physical UARTs), each
  gated on its data register actually existing for the chip actually
  being targeted (`#ifdef UDR1` etc., checked via `-mmcu`) - smaller AVR
  chips only define `UDR0`, so trying to use a UART that doesn't
  physically exist fails clearly at compile time ("`Uart1` has not been
  declared") rather than either compiling wrong or failing to compile
  at all just from including the header.
- Both are **caller-owned**: nothing in this file may self-initialize on
  first use. A lazily-self-initializing UART is a real collision hazard
  the moment more than one library shares it in the same program - the
  consuming project's own `main()`/`setup()` calls `begin()` exactly
  once, the same job `Serial.begin()` does on Arduino.

**`MemoryHAL.h`**
- `int freeMemory()` - returns free heap/stack headroom in bytes, or `-1`
  if there's no meaningful equivalent on this platform (e.g. a platform
  with virtual memory).

## Adding a new platform

1. Create `src/<platform>/` with `FlashHAL.h`, `UartHAL.h`, `MemoryHAL.h`, etc.,
   implementing the contract above.
2. Add one `#elif defined(HAL_<PLATFORM>)` branch to `BareMetalHAL.h`
   including these new files. That's the only other file this
   touches - no consuming library's code changes.
