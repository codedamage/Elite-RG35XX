#!/bin/sh
# GarlicOS Ports launcher for the input probe. Put this .sh at the SD card's
# Roms/PORTS/ root, with the InputProbe/ folder beside it.
progdir="$(dirname "$0")"
gamedir="$progdir/InputProbe"
cd "$gamedir" || exit 1
export LD_LIBRARY_PATH="$gamedir/libs:$LD_LIBRARY_PATH"
export HOME="$gamedir"
./InputProbe > run.log 2>&1
