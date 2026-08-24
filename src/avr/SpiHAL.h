#ifndef BAREMETALHAL_AVR_SPIHAL_H
#define BAREMETALHAL_AVR_SPIHAL_H

#include <stdint.h>
#include <avr/io.h>

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

// Configures this chip's hardware SPI peripheral as Master, MSB-first,
// with the given mode and clock divider. Caller-owned, like every
// other category here - nothing self-initializes. Calling this again
// with different arguments is the intended way to reconfigure between
// devices on a shared bus (e.g. a slow clock for one device's own
// initialization sequence, a faster one for its normal operation, or
// different settings entirely for a second device on the same bus).
//
// SPI pin locations (SCK/MOSI/MISO) vary across the AVR family (e.g.
// ATmega2560: SCK=PB1/MOSI=PB2/MISO=PB3; ATmega328P:
// SCK=PB5/MOSI=PB3/MISO=PB4) - this HAL does not assume any one chip's
// mapping. The caller configures SCK and MOSI as OUTPUT and MISO as
// INPUT (via GpioHAL::pinMode, using the pins for the target chip)
// before calling this. The hardware SS pin (also chip-specific) must
// likewise already be configured OUTPUT - the peripheral silently
// reverts to Slave mode if SS floats or reads LOW while configured as
// an input, per the datasheet's SPI section. Device chip-select is a
// separate, ordinary GPIO pin the caller drives directly - this HAL
// does not own it.
inline void spiBegin(SpiClockDiv div = SpiClockDiv::DIV4,
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
