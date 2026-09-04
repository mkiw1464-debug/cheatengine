#!/bin/bash
set -e

SDK=$(xcrun --sdk iphoneos --show-sdk-path)
ARCH="arm64"

echo "======================================"
echo "BUILD INFORMATION"
echo "======================================"
echo "SDK:   $SDK"
echo "ARCH:  $ARCH"
echo "THEOS: $THEOS"
echo "======================================"

# ── Resolve substrate header path ────────────────────────
# Check which layout theos vendor is using
SUBSTRATE_INC=""
if [ -d "$HOME/theos/vendor/include/CydiaSubstrate" ]; then
    SUBSTRATE_INC="-I$HOME/theos/vendor/include/CydiaSubstrate"
elif [ -f "$HOME/theos/vendor/include/substrate.h" ]; then
    SUBSTRATE_INC=""   # already covered by -I$HOME/theos/vendor/include
else
    echo "[WARN] Substrate headers not found under theos/vendor/include"
fi

CFLAGS="
  -arch $ARCH
  -isysroot $SDK
  -mios-version-min=13.0
  -std=c++17
  -O2
  -fvisibility=hidden
  -fPIC
  -I$HOME/theos/vendor/include
  $SUBSTRATE_INC
  -ISources
  -ISources/ESPFF
  -ISources/ESPFF/headers
  -ISources/ESPFF/sources
  -ISources/ESPFF/esp
  -ISources/ESPFF/esp/Core
  -ISources/ESPFF/esp/drawing_view
  -ISources/ScreenProtectorKit
  -DSUBSTRATE_AVAILABLE=1
"

LDFLAGS="
  -arch $ARCH
  -isysroot $SDK
  -dynamiclib
  -install_name @rpath/ModEngine.dylib
  -framework Foundation
  -framework UIKit
  -framework CoreGraphics
  -lsubstrate
  -L$HOME/theos/vendor/lib
"

mkdir -p build

echo "======================================"
echo "COMPILING"
echo "======================================"

clang++ $CFLAGS \
  -x objective-c++ \
  Sources/ModEngine.mm \
  Sources/MenuView.mm \
  $LDFLAGS \
  -o build/ModEngine.dylib

echo ""
echo "======================================"
echo "BUILD OK"
echo "======================================"

file build/ModEngine.dylib
ls -lh build/ModEngine.dylib
