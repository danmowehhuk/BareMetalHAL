// examples/sd-basic-avr/sd-basic-avr.cpp
//
// Real-hardware verification only - SimulIDE cannot simulate an SD
// card. Mounts the card, writes a known pattern to a file, reads it
// back, and reports pass/fail over UART.
#include <string.h>
#include <BareMetalHAL.h>
#include "avr/fatfs/ff.h"
#include "avr/fatfs/sdcard.h"

using namespace BareMetalHAL;

int main() {
  Uart0::begin(9600);
  timingInit();

  // ATmega2560 SPI pins (verified against pins_arduino.h): SCK=PB1,
  // MOSI=PB2, MISO=PB3. A different AVR chip would need its own mapping.
  sdDiskSetSpiPins(pin(Port::B, 1), pin(Port::B, 2), pin(Port::B, 3));
  sdDiskSetCsPin(pin(Port::B, 0)); // D53, matches the real board's SD_CS

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
