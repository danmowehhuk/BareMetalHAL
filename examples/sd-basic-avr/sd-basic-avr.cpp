// examples/sd-basic-avr/sd-basic-avr.cpp
//
// Real-hardware verification only - SimulIDE cannot simulate an SD
// card. Mounts the card, writes a known pattern to a file, reads it
// back, and reports pass/fail over UART.
//
// The card must already carry a FAT12/16/32 filesystem - this build
// has f_mkfs() disabled (FF_USE_MKFS=0 in ffconf.h), so it cannot
// format a raw card itself, and exFAT is unsupported (FF_FS_EXFAT=0).
// Many SDXC cards ship pre-formatted as exFAT and need reformatting to
// FAT32 (a PC or phone can do this) before this example will mount
// them. An unmountable card is not a crash: f_mount() returns a
// non-FR_OK result, printed over UART, and every file operation below
// is skipped.
#include <string.h>
#include <BareMetalHAL.h>
#include "avr/fatfs/ff.h"
#include "avr/fatfs/sdcard.h"

using namespace BareMetalHAL;

int main() {
  Uart0::begin(9600);
  timingInit();

  // This board's pins: hardware SPI SCK=PB1/MOSI=PB2/MISO=PB3 (verified
  // against pins_arduino.h), SD module's own CS=PE7 (this board's real
  // wiring - confirmed directly, not assumed from a generic Mega SD
  // shield's usual SS=PB0/D53 convention). Since CS isn't wired to the
  // chip's own hardware SS pin here, PB0 needs its own OUTPUT
  // configuration - and it must happen BEFORE spiBegin(), not after:
  // spiBegin() writes MSTR=1 to SPCR, and if SS is still an input
  // reading LOW at that exact moment, the hardware immediately clears
  // MSTR back to 0 (auto-switch to Slave mode, per the datasheet) -
  // configuring PB0 OUTPUT afterward doesn't undo that. Confirmed as a
  // real hang on real hardware from getting this ordering backwards.
  pinMode(pin(Port::B, 0), OUTPUT);
  spiBegin(pin(Port::B, 1), pin(Port::B, 2), pin(Port::B, 3));
  sdDiskSetCsPin(pin(Port::E, 7));

  FATFS fs;
  FRESULT res = f_mount(&fs, "", 1);
  Uart0::print("f_mount: ");
  Uart0::println((int)res);

  if (res == FR_OK) {
    FIL file;
    res = f_open(&file, "TEST.TXT", FA_CREATE_ALWAYS | FA_WRITE);
    Uart0::print("f_open (write): ");
    Uart0::println((int)res);

    if (res == FR_OK) {
      const char* msg = "Hello from BareMetalHAL SPI/FatFs\n";
      UINT written;
      f_write(&file, msg, strlen(msg), &written);
      Uart0::print("bytes written: ");
      Uart0::println((int)written);
      f_close(&file);
    }

    res = f_open(&file, "TEST.TXT", FA_READ);
    Uart0::print("f_open (read): ");
    Uart0::println((int)res);

    if (res == FR_OK) {
      char buf[64];
      UINT read;
      f_read(&file, buf, sizeof(buf) - 1, &read);
      buf[read] = '\0';
      Uart0::print("read back: ");
      Uart0::println(buf);
      f_close(&file);
    }
  }

  Uart0::println("Done.");
  while (true) {}
  return 0;
}
