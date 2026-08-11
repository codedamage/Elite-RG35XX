#!/bin/sh
# Task 04: build newkind (lgblgblgb) backported to SDL 1.2 for the RG35XX.
# STARTER / WIP — expect to iterate: the SDL2->1.2 shim will surface compile errors
# to fix one by one (that IS the task). Run via: docker compose run --rm engine
set -eu

CC="${CC:-arm-miyoo-linux-uclibcgnueabi-gcc}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ENGINE="$ROOT/engine/newkind"
PATCHES="$ROOT/engine/patches"
OUT="$ROOT/port/Elite/EliteTNK"

# 1. Fetch the base codebase (lgblgblgb SDL2 fork; decision in tasks/01)
if [ ! -d "$ENGINE" ]; then
  git clone --depth 1 https://github.com/lgblgblgb/newkind "$ENGINE"
fi

# 2. Stage the shim next to the sources
cp "$PATCHES/sdl12_compat.h" "$PATCHES/sdl12_compat.c" "$ENGINE/"

# 2b. Redirect the fork's bundled SDL2 gfx headers to the on-device SDL 1.2 gfx,
#     so `#include "SDL2_gfxPrimitives.h"` in the sources resolves to SDL_gfx 1.2.
printf '#include "SDL_gfxPrimitives.h"\n' > "$ENGINE/SDL2_gfxPrimitives.h"
printf '#include "SDL_rotozoom.h"\n'      > "$ENGINE/SDL2_rotozoom.h"
: > "$ENGINE/SDL2_gfxPrimitives_font.h"   # font funcs come from the SDL 1.2 gfx header

cd "$ENGINE"

# 3. Compile: drop the bundled SDL2 gfx sources, add our shim, force-include it.
#    SDL 1.2 + SDL_gfx come from the toolchain sysroot (device provides them at runtime).
SDL_CFLAGS="$( (sdl-config --cflags) 2>/dev/null || echo '')"
SDL_LIBS="$(   (sdl-config --libs)   2>/dev/null || echo '-lSDL)')"

if ! printf '%s' "$SDL_CFLAGS$SDL_LIBS" | grep -q SDL; then
  echo "WARN: sdl-config (SDL 1.2) not found in the toolchain image."
  echo "      Add SDL 1.2 + SDL_gfx dev packages to the build sysroot (or a Dockerfile) first."
fi

# Source set = fork's .c files, MINUS the bundled SDL2 gfx, PLUS our shim .c
SRCS="$(ls *.c | grep -vE '^(SDL2_gfxPrimitives|SDL2_rotozoom)\.c$')"

echo "== Compiling (WIP: fix shim errors as they appear) =="
set -x
$CC -O2 -std=c99 -I. $SDL_CFLAGS \
    -include "$ENGINE/sdl12_compat.h" \
    $SRCS \
    $SDL_LIBS -lSDL_gfx -lm \
    -o "$OUT"
set +x

echo
echo "Built: $OUT"
sh "$ROOT/scripts/check-abi.sh" "$OUT" || true
echo "Then stage data:  cp -r $ENGINE/data $ROOT/port/Elite/  (only if free to include)"
