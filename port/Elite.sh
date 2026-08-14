#!/bin/sh
# GarlicOS Ports launcher — this .sh must sit at the ROOT of the SD card's Roms/PORTS/
# (its filename is the menu label). Game files live in the sibling "Elite/" folder.
progdir="$(dirname "$0")"
gamedir="$progdir/Elite"
cd "$gamedir" || exit 1

export LD_LIBRARY_PATH="$gamedir/libs:$LD_LIBRARY_PATH"
export HOME="$gamedir"           # keep saves/config local to the game folder

# Logging is OFF by default: the game/shim print a lot, and writing that to the SD
# card every frame slows things down and wears the card. To capture a log for
# debugging, drop an empty file named "DEBUG" into the Elite/ folder.
if [ -f ./DEBUG ]; then OUT="log.txt"; else OUT="/dev/null"; fi

if [ -x ./EliteTNK ]; then
    ./EliteTNK > "$OUT" 2>&1
elif [ -x ./hello ]; then
    ./hello > hello.log 2>&1     # task 02 pipeline test
fi
