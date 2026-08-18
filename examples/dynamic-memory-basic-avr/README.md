# Why this example has three files

`examples/dynamic-memory-basic-avr/` looks more complicated than
`gpio-basic-avr/` or `timing-basic-avr/` at first glance - three source
files instead of one or two, and `build.sh` compiles all three
together. Here's what each one is for:

- **`dynamic-memory-basic-avr.cpp`** - the actual demo. Allocates an
  array with `new`, writes and sums values to prove it's real writable
  memory, frees it with `delete[]`, and prints
  `BareMetalHAL::freeMemory()` before/after over serial.
- **`src/avr/MemoryHAL.cpp`** - not example code. This is the library
  file every consumer's build must compile and link (see the main
  README's "Using dynamic memory on HAL_AVR" section) - the example's
  `build.sh` does this the same way a real consuming project's build
  script would.
- **`consumer_no_include.cpp`** - a regression test, not a pattern to
  copy. It contains a function that uses `new`/`delete[]` without
  including any `BareMetalHAL` header at all, deliberately mirroring
  how a real consumer's own code (e.g. a library whose own header never
  pulls in `BareMetalHAL.h` directly) actually looks. Its only purpose
  is proving the cross-translation-unit link genuinely works - a
  single-file example that itself includes `BareMetalHAL.h` couldn't
  catch a linkage bug that only shows up when the two are separate,
  which is exactly what happened during this category's own
  development (see the main README's `MemoryHAL.h` contract entry).

`build.sh` links all three into one `.elf` because that's what proves
all three pieces connect correctly at once - it's not evidence that a
real project needs three files to use dynamic memory. A real consumer
just needs their own code plus `MemoryHAL.cpp`.
