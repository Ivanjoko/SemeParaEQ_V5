#!/bin/bash
# Doble clic (o "bash Instalar.command") para instalar SemeParaEQ (VST3 + AU) en tu Mac.
cd "$(dirname "$0")"
mkdir -p ~/Library/Audio/Plug-Ins/VST3 ~/Library/Audio/Plug-Ins/Components

for p in *.vst3; do
  [ -e "$p" ] || continue
  rm -rf ~/Library/Audio/Plug-Ins/VST3/"$p"
  cp -R "$p" ~/Library/Audio/Plug-Ins/VST3/
  xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"$p"
done
for p in *.component; do
  [ -e "$p" ] || continue
  rm -rf ~/Library/Audio/Plug-Ins/Components/"$p"
  cp -R "$p" ~/Library/Audio/Plug-Ins/Components/
  xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/"$p"
done

killall -9 AudioComponentRegistrar 2>/dev/null
echo "SemeParaEQ instalado. En FL Studio: Options > Manage plugins > Find installed plugins."
echo "(Si tenías la versión antigua 'ParaEQ', puedes borrarla de ~/Library/Audio/Plug-Ins/VST3 y Components.)"
