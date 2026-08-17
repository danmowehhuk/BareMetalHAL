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
    GpioHAL.h
    TimingHAL.h
    TimingHAL.cpp
```

Each platform gets its own folder implementing the contract below - most
entries are headers, but a category may also contribute a `.cpp` file
where one is needed (see `TimingHAL.cpp` below). The folder *is* the
implementation; `BareMetalHAL.h` only ever routes to one of them.

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

**`GpioHAL.h`**
- `namespace BareMetalHAL { enum class Port : uint8_t { ... }; }` - one
  enumerator per physical port any chip in this platform's family can
  have (the `avr/` backend lists `A`-`L`; AVR skips `I` entirely - no
  chip defines `PORTI`). Unlike `UartHAL`'s per-UART namespaces, a
  single `Port` enum covers every chip in the family - which ports a
  specific chip actually has is handled inside the facade functions
  below, not by which names are declared, so this works the same way
  across the whole AVR family, not just the chip with the most ports.
- `constexpr uint8_t pin(Port port, uint8_t bit)` - packs a port and bit
  index into one `uint8_t`, so a pin identity can pass through consuming
  libraries' existing runtime callback function pointers (e.g.
  Eventuino's `pinSetupCallback_t = void(*)(uint8_t)`) unchanged. The
  packed value is opaque to callers - only `pin()` produces it and only
  the facade functions below consume it.
- `INPUT`, `OUTPUT`, `INPUT_PULLUP`, `HIGH`, `LOW` - mode and level
  constants. Values match Arduino's own, so a literal passed instead of
  one of these names still behaves the same.
- `void pinMode(uint8_t packedPin, uint8_t mode)`, `void
  digitalWrite(uint8_t packedPin, uint8_t value)`, `uint8_t
  digitalRead(uint8_t packedPin)` - the pin-level facade, taking a
  packed pin from `pin()` above. `pinMode`/`digitalWrite` wrap their
  register read-modify-write in `cli()`/`SREG` save-restore, matching
  the shape of Arduino's own `pinMode`/`digitalWrite` protection against
  an ISR touching the same port mid-instruction - implemented and
  syntax/link verified, but not yet exercised under live interrupt
  pressure (this PR's example never calls `sei()`, and the current
  consumer has no ISR usage), so treat it as implemented-but-unproven
  rather than a settled guarantee. `digitalRead` is a single register
  read, already atomic on AVR, so it's deliberately left unprotected.

  **Caveat - this is not the same guarantee `UartHAL` makes.** A packed
  pin identifying a port this specific chip doesn't have (e.g. `Port::L`
  packed on a smaller AVR chip that never defines `PORTL`) is a silent
  no-op in `pinMode`/`digitalWrite`, and `digitalRead` returns `LOW`.
  This is the opposite of `UartHAL`'s documented behavior, where naming a
  UART that doesn't exist "fails clearly at compile time." The
  difference is deliberate, not an oversight: a UART is selected by name
  at the call site (`BareMetalHAL::Uart1::begin(...)`), so a
  nonexistent one can simply not be declared and let the compiler catch
  it. A GPIO pin identity, by contrast, is a runtime `uint8_t` value that
  typically arrives through a consuming library's own runtime callback
  function pointer - there's no name at the call site for the compiler
  to reject, and so no compile-time hook available the way there is for
  UART. Don't assume UART's fail-fast guarantee carries over to GPIO.

**`TimingHAL.h`**
- `void timingInit()` - configures the platform's millisecond-tick
  hardware and enables interrupts. Caller-owned, same as `UartHAL`'s
  `begin()`: nothing in this file self-initializes, the consuming
  project's own `main()` calls this exactly once before `millis()` is
  used anywhere.
- `uint32_t millis()` - milliseconds elapsed since
  `timingInit()` was called, matching Arduino's own `millis()`
  semantics (same tick granularity, same `uint32_t` overflow
  behavior).
- **This category claims a hardware timer exclusively.** The `avr/`
  backend uses Timer0 - on the ATmega2560 this means a `HAL_AVR`
  consumer using `TimingHAL` cannot also use hardware PWM on pins 4
  and 13 (`TIMER0B`/`TIMER0A`). This is a verifiable consequence of the
  code claiming that timer's control registers, not an empirical
  runtime claim - stated as settled fact, the same way `UartHAL`'s
  caller-owned `begin()` requirement is stated as fact rather than
  hedged.
- **This is the only category with a `.cpp` file.** Every other category
  in this library is header-only; `TimingHAL` needs external linkage for
  the millisecond counter and its ISR, so a consuming build must compile
  and link `src/<platform>/TimingHAL.cpp` itself - adding it to the
  include path is not enough, since nothing in the header emits its
  code. See `examples/timing-basic-avr/build.sh` for a worked example of
  what that looks like in practice.

  **Caveat - `timingInit()` requires `F_CPU` to be evenly divisible by
  64000.** `avr/TimingHAL.h`'s `timingInit()` is a template on `Cpu`
  (default `F_CPU`) guarded by `static_assert(Cpu % 64000UL == 0, ...)`
  - a `static_assert` inside the template rejects anything else at the
  call site. This rules out several real `F_CPU` values, including
  20MHz, 12MHz, and a fresh ATmega328P's 1MHz factory-default fuse
  setting; verified compiling at 16MHz and 8MHz, rejected at
  20MHz/12MHz/1MHz.

## Adding a new platform

1. Create `src/<platform>/` with `FlashHAL.h`, `UartHAL.h`, `MemoryHAL.h`,
   `GpioHAL.h`, `TimingHAL.h`, etc., implementing the contract above -
   most entries are headers, but a platform may contribute source files
   as well where a category needs one (e.g. `TimingHAL.cpp`).
2. Add one `#elif defined(HAL_<PLATFORM>)` branch to `BareMetalHAL.h`
   including these new files. That's the only other file this
   touches - no consuming library's code changes.
