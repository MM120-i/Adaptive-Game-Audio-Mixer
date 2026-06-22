#!/usr/bin/env bash
set -euo pipefail

cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Debug

./build/AudioMixer_artefacts/Debug/AudioMixer.exe
