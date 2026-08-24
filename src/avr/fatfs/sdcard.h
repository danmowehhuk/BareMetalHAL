#ifndef BAREMETALHAL_AVR_FATFS_SDCARD_H
#define BAREMETALHAL_AVR_FATFS_SDCARD_H

#include <stdint.h>

// Sets which pin this driver treats as the SD card's chip-select.
// Call once, before f_mount() - FatFs's own interface has no notion of
// "which pin is chip-select", so something has to supply it before
// disk_initialize() can run.
//
// This driver does not configure the SPI bus itself (SCK/MOSI/MISO) -
// call BareMetalHAL::spiBegin() for that, once, before f_mount(). The
// bus is shared by every SPI device an application uses, not owned by
// this SD driver alone.
void sdDiskSetCsPin(uint8_t csPin);

#endif
