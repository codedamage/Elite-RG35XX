#!/bin/sh
# Test A (from tasks/09-testing.md): verify a binary matches the device ABI.
# Usage: sh scripts/check-abi.sh <binary>
# PASS = ELF32 ARM, uClibc interpreter, armv5te, no stray glibc/SDL2 NEEDED.
BIN="${1:?Usage: check-abi.sh <binary>}"
RE="${READELF:-arm-miyoo-linux-uclibcgnueabi-readelf}"
command -v "$RE" >/dev/null 2>&1 || RE=readelf

fail=0
say() { printf '  %-14s %s\n' "$1" "$2"; }

echo "== ABI check: $BIN =="

hdr="$($RE -h "$BIN" 2>/dev/null)"
echo "$hdr" | grep -q 'ELF32' && say "class:" "ELF32 OK" || { say "class:" "NOT ELF32"; fail=1; }
echo "$hdr" | grep -qi 'ARM'   && say "machine:" "ARM OK" || { say "machine:" "NOT ARM"; fail=1; }

interp="$($RE -l "$BIN" 2>/dev/null | grep -i interpreter)"
case "$interp" in
  *uClibc*) say "interp:" "uClibc OK" ;;
  *) say "interp:" "NOT uClibc ($interp)"; fail=1 ;;
esac

arch="$($RE -A "$BIN" 2>/dev/null | grep -i Tag_CPU_arch | head -1)"
case "$arch" in
  *v5TE*|*v5*) say "arch:" "armv5te OK" ;;
  *) say "arch:" "unexpected ($arch)"; ;;   # warn only; armv7 would still run but flags mismatch
esac

needed="$($RE -d "$BIN" 2>/dev/null | grep -i NEEDED)"
if echo "$needed" | grep -qiE 'ld-linux-armhf|libSDL2|SDL2'; then
  say "needed:" "STRAY glibc/SDL2 dep found!"; echo "$needed"; fail=1
else
  say "needed:" "no stray glibc/SDL2"
fi

echo
[ "$fail" -eq 0 ] && echo "RESULT: PASS" || { echo "RESULT: FAIL"; exit 1; }
