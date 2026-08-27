#ifndef BAREMETALHAL_AVR_SPIHAL_H
#define BAREMETALHAL_AVR_SPIHAL_H

#include <stdint.h>
#include <avr/io.h>
#include "GpioHAL.h"

namespace BareMetalHAL {

#ifdef SPCR

// SPI clock phase/polarity, numbered to match the datasheet's own Mode
// 0-3 terminology.
enum class SpiMode : uint8_t { MODE0, MODE1, MODE2, MODE3 };

// SPI clock divider relative to F_CPU. DIV64X2 exists only because the
// SPI2X/SPR1/SPR0 encoding has two bit patterns that both mean /64 (a
// real quirk of the register, not a naming mistake) - prefer DIV64.
enum class SpiClockDiv : uint8_t {
  DIV2, DIV4, DIV8, DIV16, DIV32, DIV64, DIV128, DIV64X2
};

// Sets SPCR/SPSR for the given clock divider and mode, without
// touching pins. Call this before talking to a particular device -
// different devices sharing a bus commonly need different settings,
// and switching between them is just this: two register writes, cheap
// enough to call before every device's own transfer(s). Pins are
// configured once, by spiBegin() below, not by this function.
inline void spiConfigure(SpiClockDiv div = SpiClockDiv::DIV4,
                          SpiMode mode = SpiMode::MODE0) {
  uint8_t spcr = (1 << SPE) | (1 << MSTR);
  switch (mode) {
    case SpiMode::MODE1: spcr |= (1 << CPHA); break;
    case SpiMode::MODE2: spcr |= (1 << CPOL); break;
    case SpiMode::MODE3: spcr |= (1 << CPOL) | (1 << CPHA); break;
    default: break;  // MODE0: CPOL=0, CPHA=0
  }

  uint8_t spsr = 0;
  switch (div) {
    case SpiClockDiv::DIV2:    spsr = (1 << SPI2X); break;
    case SpiClockDiv::DIV4:    break;
    case SpiClockDiv::DIV8:    spcr |= (1 << SPR0); spsr = (1 << SPI2X); break;
    case SpiClockDiv::DIV16:   spcr |= (1 << SPR0); break;
    case SpiClockDiv::DIV32:   spcr |= (1 << SPR1); spsr = (1 << SPI2X); break;
    case SpiClockDiv::DIV64:   spcr |= (1 << SPR1); break;
    case SpiClockDiv::DIV64X2: spcr |= (1 << SPR1) | (1 << SPR0); spsr = (1 << SPI2X); break;
    case SpiClockDiv::DIV128:  spcr |= (1 << SPR1) | (1 << SPR0); break;
  }

  SPCR = spcr;
  SPSR = spsr;
}

// One-time whole-bus setup: configures SCK/MOSI as OUTPUT and MISO as
// INPUT, then enables the SPI peripheral as Master via spiConfigure()
// above. Caller-owned, like every other category here - call once, not
// per device. Every SPI device on this bus shares these same 3 pins;
// only clock/mode (spiConfigure() above) and chip-select (each
// device's own driver) are per-device.
//
// sckPin/mosiPin/misoPin are packed GpioHAL pin identifiers (see
// GpioHAL::pin()) - which physical pins these are is chip-specific, so
// this file makes no assumption about them and states no example
// mapping; consult the target chip's own datasheet.
//
// The chip's dedicated hardware SS pin (also datasheet-specific, and
// not necessarily the same physical pin as any device's own
// chip-select) must separately be configured OUTPUT, and BEFORE this
// call, not after: this function writes MSTR=1 to SPCR, and if SS is
// still an input reading LOW at that exact moment, the hardware
// immediately clears MSTR back to 0 (auto-switch to Slave mode, per
// the datasheet's SPI section) - configuring SS OUTPUT afterward does
// not undo that once it's happened. If a device's own chip-select
// happens to be wired to that same pin (as this driver's SD example
// does) and that device's own chip-select setup already runs first,
// that's sufficient; otherwise configure SS OUTPUT directly before
// calling this. Confirmed as a real hang on real hardware from getting
// this ordering backwards, not just a theoretical concern.
inline void spiBegin(uint8_t sckPin, uint8_t mosiPin, uint8_t misoPin,
                      SpiClockDiv div = SpiClockDiv::DIV4,
                      SpiMode mode = SpiMode::MODE0) {
  pinMode(sckPin, OUTPUT);
  pinMode(mosiPin, OUTPUT);
  pinMode(misoPin, INPUT);
  spiConfigure(div, mode);
}

// Full-duplex single-byte transfer: shifts `out` onto MOSI while
// simultaneously reading whatever comes in on MISO during the same 8
// clocks, and returns it.
inline uint8_t spiTransfer(uint8_t out) {
  SPDR = out;
  while (!(SPSR & (1 << SPIF))) { }
  return SPDR;
}

#endif  // SPCR

}  // namespace BareMetalHAL

#endif
