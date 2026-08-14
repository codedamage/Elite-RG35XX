#!/bin/bash
# make-port.sh — build a drag-and-drop GarlicOS port package.
#
# Run from the repo root after cross-compiling the engine:
#   docker compose run --rm engine
#   bash scripts/make-port.sh
#
# Output: dist/Elite-RG35XX.zip
# Install: unzip Elite-RG35XX.zip onto the ROOT of the RG35XX SD card.
#          GarlicOS → Ports → Elite

set -euo pipefail

BINARY="port/Elite/EliteTNK"
DATA_DIR="port/Elite/data"
LIBS_DIR="port/Elite/libs"
LAUNCHER="port/Elite.sh"
OUT_DIR="dist"
ZIP_NAME="Elite-RG35XX.zip"

# ── Pre-flight checks ────────────────────────────────────────────────────────

if [ ! -f "$BINARY" ]; then
    echo "ERROR: $BINARY not found."
    echo "       Run 'docker compose run --rm engine' first to cross-compile."
    exit 1
fi

if [ ! -f "$LAUNCHER" ]; then
    echo "ERROR: $LAUNCHER not found."
    exit 1
fi

# Verify it's an ARM ELF (not a host binary)
if command -v file >/dev/null 2>&1; then
    file_out=$(file "$BINARY")
    if ! echo "$file_out" | grep -qi "arm"; then
        echo "WARNING: $BINARY does not appear to be an ARM binary:"
        echo "         $file_out"
        echo "         Continue anyway? (y/N)"
        read -r ans
        [ "$ans" = "y" ] || [ "$ans" = "Y" ] || exit 1
    fi
fi

# ── Build GarlicOS directory tree in a temp dir ──────────────────────────────

TMP_DIR=$(mktemp -d)
trap 'rm -rf "$TMP_DIR"' EXIT

PORTS_DIR="$TMP_DIR/Roms/PORTS"
GAME_DIR="$PORTS_DIR/Elite"
mkdir -p "$GAME_DIR"

echo "Assembling port package..."

# Launcher (must sit at Roms/PORTS/ root, filename = menu label)
cp "$LAUNCHER" "$PORTS_DIR/Elite.sh"
chmod +x "$PORTS_DIR/Elite.sh"

# Binary
cp "$BINARY" "$GAME_DIR/EliteTNK"
chmod +x "$GAME_DIR/EliteTNK"

# Data files (game assets — only copy if present and non-empty)
if [ -d "$DATA_DIR" ] && [ -n "$(ls -A "$DATA_DIR" 2>/dev/null)" ]; then
    cp -r "$DATA_DIR" "$GAME_DIR/data"
    echo "  + data/  ($(find "$GAME_DIR/data" -type f | wc -l) files)"
else
    echo "  ! data/  — empty or missing; copy game data files to $DATA_DIR before packaging"
fi

# Bundled libs (none on the SDL 1.2 path, but included if present)
if [ -d "$LIBS_DIR" ] && [ -n "$(ls -A "$LIBS_DIR" 2>/dev/null)" ]; then
    cp -r "$LIBS_DIR" "$GAME_DIR/libs"
    echo "  + libs/  ($(find "$GAME_DIR/libs" -type f | wc -l) files)"
fi

# ── Zip ──────────────────────────────────────────────────────────────────────

mkdir -p "$OUT_DIR"
OUTPUT="$OUT_DIR/$ZIP_NAME"
rm -f "$OUTPUT"

(cd "$TMP_DIR" && zip -r "$OLDPWD/$OUTPUT" Roms/ -x "*.DS_Store" -x "__MACOSX/*")

# ── Summary ──────────────────────────────────────────────────────────────────

BINARY_SIZE=$(du -sh "$BINARY" | cut -f1)
ZIP_SIZE=$(du -sh "$OUTPUT" | cut -f1)

echo ""
echo "Done!"
echo "  Package : $OUTPUT  ($ZIP_SIZE)"
echo "  Binary  : $BINARY_SIZE"
echo ""
echo "To install:"
echo "  1. Insert your RG35XX SD card."
echo "  2. Unzip $ZIP_NAME onto the SD card ROOT."
echo "     (The archive already contains Roms/PORTS/Elite.sh and Roms/PORTS/Elite/)"
echo "  3. Safely eject, insert into RG35XX."
echo "  4. GarlicOS → Ports → Elite"
echo ""
echo "Note: game data files must be present in Roms/PORTS/Elite/data/ for the game to run."
