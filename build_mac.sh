#!/bin/bash
# Compila SemeParaEQ para macOS (Apple Silicon + Intel) y lo instala.
set -e
cd "$(dirname "$0")"
command -v cmake >/dev/null || { echo "Falta CMake:  brew install cmake"; exit 1; }
xcode-select -p >/dev/null 2>&1 || { echo "Faltan las herramientas de Xcode:  xcode-select --install"; exit 1; }

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j "$(sysctl -n hw.ncpu)"

for p in ~/Library/Audio/Plug-Ins/VST3/SemeParaEQ.vst3 ~/Library/Audio/Plug-Ins/Components/SemeParaEQ.component; do
  [ -e "$p" ] && codesign --force --deep -s - "$p" && echo "Firmado: $p"
done
killall -9 AudioComponentRegistrar 2>/dev/null || true
echo "Listo. En FL Studio: Options > Manage plugins > Find installed plugins."
