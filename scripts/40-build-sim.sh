#!/bin/bash
# Headless SIMULATOR: build the port (our SDL 1.2 shim) natively on x86, run it with
# the SDL "dummy" video driver, and dump frames to sim/*.png so we can see the display
# and iterate WITHOUT the device. Run via: docker compose run --rm sim
set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ENGINE="$ROOT/engine/newkind"
PATCHES="$ROOT/engine/patches"
SIMDIR="$ROOT/sim"
BIN=/tmp/EliteSim
mkdir -p "$SIMDIR"; rm -f "$SIMDIR"/*.bmp "$SIMDIR"/*.png 2>/dev/null || true

# 1. Source (reuse the clone made by the device build; clone if missing)
[ -d "$ENGINE" ] || git clone --depth 1 https://github.com/lgblgblgb/newkind "$ENGINE"

# 2. Stage shim + neutralize bundled SDL2 gfx headers + source tweaks (same as device build)
cp "$PATCHES/sdl12_compat.h" "$PATCHES/sdl12_compat.c" "$ENGINE/"
printf '#include "SDL_gfxPrimitives.h"\n' > "$ENGINE/SDL2_gfxPrimitives.h"
printf '#include "SDL_rotozoom.h"\n'      > "$ENGINE/SDL2_rotozoom.h"
: > "$ENGINE/SDL2_gfxPrimitives_font.h"
cd "$ENGINE"
sed -i 's/event\.key\.repeat/0/g' *.c
[ -f data/datafile.sh ] && bash data/datafile.sh > datafilebank.c

# Aspect ratio: render at 800x600 (4:3) so it scales to 640x480 (4:3) as a clean
# uniform 0.8x with no horizontal stretch. Disable both in the header and force
# RES_800_600 via -D (below) to avoid include-order ambiguity.
sed -i 's|^#define RES_512_512|// #define RES_512_512|' etnk.h
sed -i 's|^#define RES_800_600|// #define RES_800_600|' etnk.h
grep -nE 'RES_(512_512|800_600)' etnk.h || true

# SIM debug: log when the intro-relevant kbd_* flags go non-zero (added after they're set)
sed -i 's|kbd_space_pressed = key\[KEY_SPACE\];|kbd_space_pressed = key[KEY_SPACE]; if(kbd_y_pressed+kbd_n_pressed+kbd_space_pressed) fprintf(stderr,"DBG-KBD y=%d n=%d space=%d\\n",kbd_y_pressed,kbd_n_pressed,kbd_space_pressed);|' keyboard.c
grep -n 'DBG-KBD' keyboard.c || echo "WARN: DBG-KBD sed did not match"

# SIM debug: per-frame game state (are we launching into flight?)
sed -i 's|handle_flight_keys ();|handle_flight_keys (); fprintf(stderr,"DBG-STATE docked=%d screen=%d\\n",docked,current_screen);|' main.c
grep -c 'DBG-STATE' main.c

# 3. Native x86 build with SIM_BUILD (frame capture + scripted input in the shim)
SRCS="$(ls *.c | grep -vE '^(SDL2_gfxPrimitives|SDL2_rotozoom)\.c$')"
echo "== Building simulator (native x86, SDL 1.2) =="
gcc -O2 -std=c99 -I. -DSIM_BUILD -DRES_800_600 $(sdl-config --cflags) \
    -include "$ENGINE/sdl12_compat.h" \
    $SRCS \
    $(sdl-config --libs) -lSDL_gfx -lm \
    -o "$BIN"

# 4. Run headless: dummy video/audio drivers, no display needed. Shim dumps BMPs + exits.
#    Capture all game+shim output to sim/run.log so we can see key events, dims, errors.
echo "== Running simulator (headless) -> sim/run.log =="
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout 30 "$BIN" > "$SIMDIR/run.log" 2>&1 || true
echo "--- last 25 lines of run.log ---"; tail -25 "$SIMDIR/run.log" 2>/dev/null || true

# 5. Convert dumped BMP frames -> PNG for viewing
if ls "$SIMDIR"/*.bmp >/dev/null 2>&1; then
    for b in "$SIMDIR"/*.bmp; do convert "$b" "${b%.bmp}.png" && rm -f "$b"; done
    echo "== Frames written to sim/ =="; ls -la "$SIMDIR"/*.png
else
    echo "No frames captured - check the run output above."
fi
