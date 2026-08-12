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

cd "$ENGINE"

# 2a. Reset the fork's tracked sources to pristine so tweaks from OTHER builds (e.g. the
#     sim's DBG-KBD/DBG-STATE) never leak into the device binary. Shim + generated files
#     are untracked and survive.
git checkout -- . 2>/dev/null || true

# 2b. Stage the shim next to the sources.
cp "$PATCHES/sdl12_compat.h" "$PATCHES/sdl12_compat.c" .

# 2c. Redirect the fork's bundled SDL2 gfx headers to the on-device SDL 1.2 gfx.
printf '#include "SDL_gfxPrimitives.h"\n' > SDL2_gfxPrimitives.h
printf '#include "SDL_rotozoom.h"\n'      > SDL2_rotozoom.h
: > SDL2_gfxPrimitives_font.h

# 2d. Source tweaks:
sed -i 's/event\.key\.repeat/0/g' *.c                            # no 'repeat' field in SDL 1.2
sed -i '/puts("gfx_update_screen() is called!")/d' sdl.c         # per-frame log I/O = slow

# 2e. Embedded data bank (bmp/wav -> C arrays; not in git).
if [ -f data/datafile.sh ]; then
  bash data/datafile.sh > datafilebank.c
  echo "Generated datafilebank.c ($(wc -c < datafilebank.c) bytes)"
else
  echo "WARN: data/datafile.sh not found - datafilebank.c cannot be generated"
fi

# 2f. Render at 800x600 (4:3) -> clean uniform 0.8x to 640x480. Forced via -DRES_800_600.
sed -i 's|^#define RES_512_512|// #define RES_512_512|' etnk.h
sed -i 's|^#define RES_800_600|// #define RES_800_600|' etnk.h
grep -nE 'RES_(512_512|800_600)' etnk.h || true

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
$CC -O2 -std=c99 -I. -DRES_800_600 $SDL_CFLAGS \
    -include "$ENGINE/sdl12_compat.h" \
    $SRCS \
    $SDL_LIBS -lSDL_gfx -lm \
    -o "$OUT"
set +x

echo
echo "Built: $OUT"
sh "$ROOT/scripts/check-abi.sh" "$OUT" || true
echo
echo "EliteTNK is SELF-CONTAINED (bmp/wav embedded; elite.dat load is #if 0 dead code)."
echo "Deploy: copy BOTH  port/Elite.sh  and  port/Elite/  into the SD card's Roms/PORTS/,"
echo "then launch 'Elite' from Garlic -> Ports. Config/saves write into the Elite/ folder."
