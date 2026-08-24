#ifndef BAREMETALHAL_AVR_SPIHAL_H
#define BAREMETALHAL_AVR_SPIHAL_H

#include <stdint.h>
#include <avr/io.h>

namespace BareMetalHAL {

// Configures the ATmega's hardware SPI peripheral as Master, SPI Mode 0,
// MSB-first, clock = F_CPU/4. Caller-owned, like every other category
// here - nothing self-initializes. The hardware SS pin (PB0 on the
// ATmega2560, verified against pins_arduino.h) must already be
// configured OUTPUT before calling this - the peripheral silently
// reverts to Slave mode if SS floats or reads LOW while configured as
// an input, per the datasheet's SPI section. Device chip-select is a
// separate, ordinary GPIO pin the caller drives directly - this HAL
// does not own it.
inline void spiBegin() {
  DDRB |= (1 << DDB1) | (1 << DDB2); // SCK, MOSI as outputs
  DDRB &= ~(1 << DDB3);              // MISO as input
  SPCR = (1 << SPE) | (1 << MSTR);   // enable, master, mode 0, F_CPU/4
  SPSR = 0;
}

// Full-duplex single-byte transfer: shifts `out` onto MOSI while
// simultaneously reading whatever comes in on MISO during the same 8
// clocks, and returns it.
inline uint8_t spiTransfer(uint8_t out) {
  SPDR = out;
  while (!(SPSR & (1 << SPIF))) { }
  return SPDR;
}

}  // namespace BareMetalHAL

#endif
