#!/bin/sh
# Cross-compile the on-device input probe (SDL 1.2 + SDL_gfx) for the RG35XX.
# Run via: docker compose run --rm probe
# Output: port/InputProbe/InputProbe (+ launcher port/InputProbe.sh)
set -eu
CC="${CC:-arm-miyoo-linux-uclibcgnueabi-gcc}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/port/InputProbe"
mkdir -p "$OUT"

SDL_CFLAGS="$( (sdl-config --cflags) 2>/dev/null || echo '' )"
SDL_LIBS="$(   (sdl-config --libs)   2>/dev/null || echo '-lSDL' )"

echo "== Building input probe =="
$CC -O2 -std=c99 -I. $SDL_CFLAGS "$ROOT/scripts/input_probe.c" \
    $SDL_LIBS -lSDL_gfx -lm -o "$OUT/InputProbe"

sh "$ROOT/scripts/check-abi.sh" "$OUT/InputProbe" || true
echo
echo "Deploy: copy BOTH port/InputProbe.sh AND port/InputProbe/ to the SD card's Roms/PORTS/."
echo "Launch 'InputProbe', press every button/dpad dir, then send Roms/PORTS/InputProbe/probe.log"
