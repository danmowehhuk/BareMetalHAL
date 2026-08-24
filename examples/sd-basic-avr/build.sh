#!/bin/bash

# Usage:
#   ./build.sh        Build a .hex suitable for flashing to real hardware
#   ./build.sh -s     Build a .hex suitable for SimulIDE simulation
#
# Compile/link smoke test for the vendored FatFs + SD-over-SPI diskio
# driver - proves it all compiles and links against BareMetalHAL's own
# GpioHAL/TimingHAL/SpiHAL. SimulIDE cannot simulate an SD card, so -s
# still produces a hex but it is not a substitute for real-hardware
# verification (see the example's own comment).
#
# src/avr contains both C++ (BareMetalHAL's HAL headers/.cpp, diskio.cpp)
# and plain C (ff.c, ffunicode.c - vendored FatFs) sources, so unlike
# this repo's other -avr examples, this one archives src/avr into a
# static library first (build_archive below), compiling .cpp with
# avr-g++ and .c with avr-gcc, then links the example entry point
# against that archive.

set -euo pipefail

SIM_MODE=false
while getopts "s" opt; do
  case $opt in
    s) SIM_MODE=true ;;
  esac
done

find_avr_tool() {
  local tool="$1"
  local found

  if command -v "$tool" >/dev/null 2>&1; then
    command -v "$tool"
    return
  fi

  local search_roots=(
    "$HOME/Library/Arduino15/packages/arduino/tools/avr-gcc"   # macOS
    "$HOME/.arduino15/packages/arduino/tools/avr-gcc"          # Linux
    "$HOME/.platformio/packages/toolchain-atmelavr"
    "/opt/homebrew/opt/avr-gcc"
    "/opt/homebrew/Cellar/avr-gcc"
    "/usr/local/opt/avr-gcc"
    "/usr/local/Cellar/avr-gcc"
    "/opt/local"
    "/usr/local/avr"
    "/opt/avr"
    "/usr/avr"
  )

  for root in "${search_roots[@]}"; do
    found=$(find "$root" -name "$tool" -type f 2>/dev/null | sort -V | tail -1)
    if [ -n "$found" ]; then echo "$found"; return; fi
  done

  echo "ERROR: $tool not found on PATH or in any of the usual install locations (Arduino15, PlatformIO, Homebrew, MacPorts, /usr/local/avr, /opt/avr, /usr/avr)" >&2
  exit 1
}

AVRGXX="$(find_avr_tool avr-g++)"
AVRGCC="$(find_avr_tool avr-gcc)"
AVRAR="$(find_avr_tool avr-ar)"
AVROBJCOPY="$(find_avr_tool avr-objcopy)"
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$DIR/build"
OBJ_DIR="$BUILD_DIR/obj"
BAREMETALHAL_SRC="$DIR/../../src"

COMMON_FLAGS=(-Wall -Wextra -Os -DNO_ARDUINO -DHAL_AVR -DF_CPU=16000000UL -mmcu=atmega2560 -ffunction-sections -fdata-sections -I "$BAREMETALHAL_SRC")
CXXFLAGS=(-std=gnu++11 "${COMMON_FLAGS[@]}")
CFLAGS=(-std=gnu99 "${COMMON_FLAGS[@]}")

mkdir -p "$OBJ_DIR"

# build_archive <name> <src-root>
#
# Compiles every *.cpp (avr-g++) and *.c (avr-gcc) found recursively
# under <src-root> and archives the resulting objects into
# $BUILD_DIR/lib<name>.a. Prints the archive path.
build_archive() {
  local name="$1"
  local src_root="$2"
  local objdir="$OBJ_DIR/$name"
  mkdir -p "$objdir"

  local objs=()
  local src rel obj

  while IFS= read -r -d '' src; do
    rel="${src#"$src_root"/}"
    obj="$objdir/${rel//\//_}.o"
    "$AVRGXX" "${CXXFLAGS[@]}" -c "$src" -o "$obj"
    objs+=("$obj")
  done < <(find "$src_root" -name '*.cpp' -print0 | sort -z)

  while IFS= read -r -d '' src; do
    rel="${src#"$src_root"/}"
    obj="$objdir/${rel//\//_}.o"
    "$AVRGCC" "${CFLAGS[@]}" -c "$src" -o "$obj"
    objs+=("$obj")
  done < <(find "$src_root" -name '*.c' -print0 | sort -z)

  local archive="$BUILD_DIR/lib${name}.a"
  rm -f "$archive"
  "$AVRAR" rcs "$archive" "${objs[@]}"
  echo "$archive"
}

# Scoped to avr/ specifically (not all of BareMetalHAL's src/) - a future
# platform folder (e.g. src/esp32/) wouldn't compile under avr-g++.
build_archive baremetalhal "$BAREMETALHAL_SRC/avr" >/dev/null

"$AVRGXX" "${CXXFLAGS[@]}" \
  "$DIR/sd-basic-avr.cpp" \
  -o "$BUILD_DIR/sd-basic-avr.elf" \
  -Wl,--gc-sections \
  -L "$BUILD_DIR" -lbaremetalhal

"$AVROBJCOPY" -O ihex -R .eeprom "$BUILD_DIR/sd-basic-avr.elf" "$BUILD_DIR/sd-basic-avr.hex"

echo "Built $BUILD_DIR/sd-basic-avr.hex"

if $SIM_MODE; then
  HEX="$BUILD_DIR/sd-basic-avr.hex"
  SIM_HEX="${HEX%.hex}.sim.hex"
  python3 - "$HEX" "$SIM_HEX" << 'EOF'
import sys

def checksum(data_bytes):
    return (0x100 - sum(data_bytes) % 0x100) % 0x100

with open(sys.argv[1]) as f_in, open(sys.argv[2], 'w') as f_out:
    for line in f_in:
        line = line.strip()
        if line[7:9] == '02':  # Extended Segment Address record
            segment = int(line[9:13], 16)
            upper16 = segment >> 12
            b = [0x02, 0x00, 0x00, 0x04, upper16 >> 8, upper16 & 0xFF]
            f_out.write(f':{b[0]:02X}{b[1]:02X}{b[2]:02X}{b[3]:02X}{b[4]:02X}{b[5]:02X}{checksum(b):02X}\n')
        else:
            f_out.write(line + '\n')
EOF
  echo "SimulIDE-compatible hex: $SIM_HEX"
fi
