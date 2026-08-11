#!/bin/sh
# GarlicOS Ports launcher — this .sh must sit at the ROOT of the SD card's Roms/PORTS/
# (its filename is the menu label). Game files live in the sibling "Elite/" folder.
progdir="$(dirname "$0")"
gamedir="$progdir/Elite"
cd "$gamedir" || exit 1

export LD_LIBRARY_PATH="$gamedir/libs:$LD_LIBRARY_PATH"
export HOME="$gamedir"           # keep saves/config/logs local to the game folder

if [ -x ./EliteTNK ]; then
    ./EliteTNK > log.txt 2>&1
elif [ -x ./hello ]; then
    ./hello > hello.log 2>&1     # task 02 pipeline test
fi
