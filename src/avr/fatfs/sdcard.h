#ifndef BAREMETALHAL_AVR_FATFS_SDCARD_H
#define BAREMETALHAL_AVR_FATFS_SDCARD_H

#include <stdint.h>

// Sets which pin this driver treats as the SD card's chip-select.
// Call once, before f_mount() - FatFs's own interface has no notion of
// "which pin is chip-select", so something has to supply it before
// disk_initialize() can run.
void sdDiskSetCsPin(uint8_t csPin);

// Sets which pins this driver treats as the SPI bus's SCK/MOSI/MISO.
// Call once, before f_mount(). SPI pin locations vary across the AVR
// family (see SpiHAL.h) - this driver does not assume any one chip's
// mapping, so the caller supplies it, the same way it supplies CS.
void sdDiskSetSpiPins(uint8_t sckPin, uint8_t mosiPin, uint8_t misoPin);

#endif
