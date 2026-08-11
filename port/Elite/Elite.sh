#!/bin/sh
# GarlicOS port launcher. Place port/Elite/ under the SD card's Roms/PORTS/.
DIR="$(dirname "$0")"
cd "$DIR" || exit 1

# Bundled non-stock libs (none needed for the SDL 1.2 backport path) + local saves/config.
export LD_LIBRARY_PATH="$DIR/libs:$LD_LIBRARY_PATH"
export HOME="$DIR"

# --- Task 02: prove the pipeline ---
if [ -x "$DIR/EliteTNK" ]; then
    ./EliteTNK > log.txt 2>&1
elif [ -x "$DIR/hello" ]; then
    ./hello > hello.log 2>&1
fi
