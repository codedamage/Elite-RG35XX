#!/bin/sh
# Task 02: cross-compile hello.c for the original RG35XX (GarlicOS / Miyoo uClibc).
# Run via: docker compose run --rm hello
set -eu

CC="${CC:-arm-miyoo-linux-uclibcgnueabi-gcc}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT_DIR="$ROOT/port/Elite"
mkdir -p "$OUT_DIR"

# Use the toolchain's device-matched defaults (uClibc, armv5te, soft-float). -O2 only.
CFLAGS="-O2"

echo "CC     = $CC"
"$CC" $CFLAGS "$ROOT/scripts/hello.c" -o "$OUT_DIR/hello"

echo "Built: $OUT_DIR/hello"
sh "$ROOT/scripts/check-abi.sh" "$OUT_DIR/hello" || true
echo
echo "Next: copy port/Elite/ to the SD card's Roms/PORTS/, launch via Garlic -> Ports,"
echo "then check that hello.log appears next to the binary."
