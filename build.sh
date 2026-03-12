#!/bin/bash
# Build script for Hero Shooter V1
# Cross-compiles from Linux to Windows using Zig

set -e

ZIG="/sessions/elegant-wonderful-cerf/tools/zig-linux-x86_64-0.11.0/zig"
SDL2="/sessions/elegant-wonderful-cerf/tools/SDL2-2.30.1/x86_64-w64-mingw32"
SRC="/sessions/elegant-wonderful-cerf/game/src"
OUT="/sessions/elegant-wonderful-cerf/game/build"

mkdir -p "$OUT"

echo "Compiling Hero Shooter V1 for Windows..."

$ZIG cc -target x86_64-windows-gnu \
    -O2 \
    -I"$SDL2/include" \
    -I"$SRC" \
    -L"$SDL2/lib" \
    "$SRC/main.c" \
    "$SRC/engine/renderer.c" \
    "$SRC/engine/input.c" \
    "$SRC/game/player.c" \
    "$SRC/game/arena.c" \
    -lSDL2 \
    -lopengl32 \
    -lgdi32 \
    -lwinmm \
    -limm32 \
    -lole32 \
    -loleaut32 \
    -lsetupapi \
    -lversion \
    -lshell32 \
    -luser32 \
    -o "$OUT/HeroShooter.exe" \
    2>&1

echo "Build complete!"
echo "Output: $OUT/HeroShooter.exe"

# Copy SDL2.dll alongside the exe
cp "$SDL2/bin/SDL2.dll" "$OUT/"
echo "Copied SDL2.dll"

ls -la "$OUT/"
