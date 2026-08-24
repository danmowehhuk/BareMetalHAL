// diskio.cpp
//
// Low-level SD-over-SPI block driver, implementing the 5-function
// interface FatFs's ff.c expects (diskio.h). Speaks the standard SD
// card SPI-mode protocol directly: CMD0/CMD8/ACMD41 initialization,
// CMD58 to detect block- vs byte-addressing cards, CMD17/CMD24 for
// single-block read/write. MMC (non-SD) cards are not supported.
#include "diskio.h"
#include "ff.h"
#include "sdcard.h"
#include <BareMetalHAL.h>

using namespace BareMetalHAL;

namespace {

constexpr uint8_t CMD0   = 0;
constexpr uint8_t CMD8   = 8;
constexpr uint8_t CMD16  = 16;
constexpr uint8_t CMD17  = 17;
constexpr uint8_t CMD24  = 24;
constexpr uint8_t CMD55  = 55;
constexpr uint8_t CMD58  = 58;
constexpr uint8_t ACMD41 = 41;

constexpr uint8_t CT_SD1   = 0x01;
constexpr uint8_t CT_SD2   = 0x02;
constexpr uint8_t CT_BLOCK = 0x04;

// SD's own spec requires <=400kHz during CMD0-ACMD41 identification;
// DIV128 (F_CPU/128) satisfies that at every F_CPU this family targets.
// Once a card is identified, block I/O runs at the same DIV4 rate this
// driver always used.
constexpr SpiClockDiv kInitClockDiv = SpiClockDiv::DIV128;
constexpr SpiClockDiv kDataClockDiv = SpiClockDiv::DIV4;

uint8_t g_csPin;
uint8_t g_sckPin;
uint8_t g_mosiPin;
uint8_t g_misoPin;
DSTATUS g_status = STA_NOINIT;
uint8_t g_cardType = 0;

// Reclaims the SPI bus at this driver's own required settings - cheap
// (one register write), and correct even if something else configured
// the shared bus differently since this driver last used it.
void reclaimBus() { spiBegin(kDataClockDiv, SpiMode::MODE0); }

void select()   { digitalWrite(g_csPin, LOW); }
void deselect() {
  digitalWrite(g_csPin, HIGH);
  spiTransfer(0xFF);
}

uint8_t waitReady(uint16_t timeoutMs) {
  uint8_t res;
  uint32_t start = millis();
  do {
    res = spiTransfer(0xFF);
  } while (res != 0xFF && (millis() - start) < timeoutMs);
  return res;
}

uint8_t sendCommand(uint8_t cmd, uint32_t arg) {
  if (cmd == ACMD41) {
    uint8_t r = sendCommand(CMD55, 0);
    if (r > 1) return r;
  }

  deselect();
  select();
  if (waitReady(500) != 0xFF) return 0xFF;

  uint8_t crc = 0x01;
  if (cmd == CMD0) crc = 0x95;
  if (cmd == CMD8) crc = 0x87;

  spiTransfer(0x40 | cmd);
  spiTransfer((uint8_t)(arg >> 24));
  spiTransfer((uint8_t)(arg >> 16));
  spiTransfer((uint8_t)(arg >> 8));
  spiTransfer((uint8_t)arg);
  spiTransfer(crc);

  uint8_t res;
  uint8_t retries = 10;
  do {
    res = spiTransfer(0xFF);
  } while ((res & 0x80) && --retries);
  return res;
}

bool waitDataToken() {
  uint32_t start = millis();
  uint8_t token;
  do {
    token = spiTransfer(0xFF);
  } while (token == 0xFF && (millis() - start) < 200);
  return token == 0xFE;
}

}  // namespace

void sdDiskSetCsPin(uint8_t csPin) {
  g_csPin = csPin;
}

void sdDiskSetSpiPins(uint8_t sckPin, uint8_t mosiPin, uint8_t misoPin) {
  g_sckPin = sckPin;
  g_mosiPin = mosiPin;
  g_misoPin = misoPin;
}

extern "C" {

DSTATUS disk_status(BYTE) {
  return g_status;
}

DSTATUS disk_initialize(BYTE) {
  pinMode(g_sckPin, OUTPUT);
  pinMode(g_mosiPin, OUTPUT);
  pinMode(g_misoPin, INPUT);
  pinMode(g_csPin, OUTPUT);
  digitalWrite(g_csPin, HIGH);
  spiBegin(kInitClockDiv, SpiMode::MODE0);

  delay(10);
  for (uint8_t i = 0; i < 10; i++) spiTransfer(0xFF);

  g_cardType = 0;
  if (sendCommand(CMD0, 0) == 1) {
    uint32_t start = millis();
    if (sendCommand(CMD8, 0x1AA) == 1) {
      uint8_t r7[4];
      for (uint8_t i = 0; i < 4; i++) r7[i] = spiTransfer(0xFF);
      if (r7[2] == 0x01 && r7[3] == 0xAA) {
        while (sendCommand(ACMD41, 1UL << 30) != 0 && (millis() - start) < 1000) { }
        if ((millis() - start) < 1000 && sendCommand(CMD58, 0) == 0) {
          uint8_t ocr[4];
          for (uint8_t i = 0; i < 4; i++) ocr[i] = spiTransfer(0xFF);
          g_cardType = (ocr[0] & 0x40) ? (CT_SD2 | CT_BLOCK) : CT_SD2;
        }
      }
    } else if (sendCommand(ACMD41, 0) <= 1) {
      g_cardType = CT_SD1;
      while (sendCommand(ACMD41, 0) != 0 && (millis() - start) < 1000) { }
      if ((millis() - start) < 1000) sendCommand(CMD16, 512);
      else g_cardType = 0;
    }
    // else: MMC or unrecognized card - not supported, g_cardType stays 0
  }
  deselect();

  g_status = g_cardType ? 0 : STA_NOINIT;
  if (g_status == 0) reclaimBus();  // switch from init-phase clock to data-phase clock
  return g_status;
}

DRESULT disk_read(BYTE, BYTE* buff, LBA_t sector, UINT count) {
  if (g_status & STA_NOINIT) return RES_NOTRDY;
  reclaimBus();
  uint32_t addr = (g_cardType & CT_BLOCK) ? sector : sector * 512;

  for (UINT block = 0; block < count; block++) {
    if (sendCommand(CMD17, addr + ((g_cardType & CT_BLOCK) ? block : block * 512)) != 0) {
      deselect();
      return RES_ERROR;
    }
    select();
    if (!waitDataToken()) {
      deselect();
      return RES_ERROR;
    }
    for (UINT i = 0; i < 512; i++) buff[block * 512 + i] = spiTransfer(0xFF);
    spiTransfer(0xFF);
    spiTransfer(0xFF); // discard CRC (unused in SPI mode)
    deselect();
  }
  return RES_OK;
}

DRESULT disk_write(BYTE, const BYTE* buff, LBA_t sector, UINT count) {
  if (g_status & STA_NOINIT) return RES_NOTRDY;
  reclaimBus();
  uint32_t addr = (g_cardType & CT_BLOCK) ? sector : sector * 512;

  for (UINT block = 0; block < count; block++) {
    if (sendCommand(CMD24, addr + ((g_cardType & CT_BLOCK) ? block : block * 512)) != 0) {
      deselect();
      return RES_ERROR;
    }
    select();
    spiTransfer(0xFE); // data start token
    for (UINT i = 0; i < 512; i++) spiTransfer(buff[block * 512 + i]);
    spiTransfer(0xFF);
    spiTransfer(0xFF); // dummy CRC

    uint8_t resp = spiTransfer(0xFF);
    if ((resp & 0x1F) != 0x05) { // data response token, 0bxxx00101 = accepted
      deselect();
      return RES_ERROR;
    }
    if (waitReady(500) != 0xFF) {
      deselect();
      return RES_ERROR;
    }
    deselect();
  }
  return RES_OK;
}

DRESULT disk_ioctl(BYTE, BYTE cmd, void* buff) {
  if (g_status & STA_NOINIT) return RES_NOTRDY;
  switch (cmd) {
    case CTRL_SYNC:
      reclaimBus();
      select();
      if (waitReady(500) == 0xFF) { deselect(); return RES_OK; }
      deselect();
      return RES_ERROR;
    case GET_SECTOR_SIZE:
      *(WORD*)buff = 512;
      return RES_OK;
    default:
      return RES_PARERR;
  }
}

DWORD get_fattime() {
  // FF_FS_NORTC=1 handles the no-RTC case inside ff.c itself - this
  // function only exists to satisfy the linker if ff.c is built without
  // that config path; not expected to actually be called.
  return 0;
}

}  // extern "C"
