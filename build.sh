#!/bin/bash
# Build Cadence. Run from the project root.
#   ./build.sh          -> meson/ninja build, binary at build/cadence
#   ./build.sh gcc      -> plain gcc build, binary at ./cadence
# Needs: libgtk-4-dev libsdl2-dev  (sudo apt install libgtk-4-dev libsdl2-dev meson ninja-build)
set -e
cd "$(dirname "$0")"

if [ "$1" != "gcc" ] && command -v meson >/dev/null && command -v ninja >/dev/null; then
    [ -d build/meson-private ] && meson setup --reconfigure build >/dev/null || meson setup build
    ninja -C build
    echo "=== Built: build/cadence ==="
    echo "Run:  ./build/cadence"
else
    gcc -Wall -O2 -o cadence src/main.c $(pkg-config --cflags --libs gtk4 sdl2 pangocairo) -lm
    echo "=== Built: ./cadence ==="
    echo "Run:  ./cadence"
fi
