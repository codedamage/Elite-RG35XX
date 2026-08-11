#!/bin/sh
# Probe the toolchain image for the SDL 1.2 + SDL_gfx DEV files the engine build (task 04) needs.
# Run: docker compose run --rm shell sh scripts/check-toolchain-sdl.sh
echo "== SDL 1.2 dev in toolchain? =="

if command -v sdl-config >/dev/null 2>&1; then
  echo "sdl-config: FOUND  (version $(sdl-config --version), prefix $(sdl-config --prefix))"
  echo "  cflags: $(sdl-config --cflags)"
  echo "  libs:   $(sdl-config --libs)"
else
  echo "sdl-config: MISSING  -> SDL 1.2 dev not installed in the image"
fi

echo
echo "-- headers --"
for h in SDL/SDL.h SDL/SDL_gfxPrimitives.h SDL/SDL_rotozoom.h \
         SDL.h SDL_gfxPrimitives.h SDL_rotozoom.h; do
  f="$(find / -name "$(basename "$h")" 2>/dev/null | head -1)"
  [ -n "$f" ] && echo "  OK   $(basename "$h")  ->  $f" || echo "  ----  $(basename "$h")  NOT FOUND"
done

echo
echo "-- libs --"
for l in libSDL libSDL_gfx; do
  f="$(find / -name "${l}*.so*" -o -name "${l}*.a" 2>/dev/null | head -3)"
  [ -n "$f" ] && { echo "  OK   $l:"; echo "$f" | sed 's/^/       /'; } || echo "  ----  $l NOT FOUND"
done

echo
echo "Interpretation:"
echo "  * All FOUND  -> task 04 can build against SDL 1.2 + SDL_gfx directly."
echo "  * SDL_gfx missing -> add it to the build sysroot (cross-build SDL_gfx 1.2), OR compile the"
echo "    fork's bundled gfx sources adapted to 1.2. SDL 1.2 core is the must-have."
