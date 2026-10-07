#!/usr/bin/env bash
# Compile-check src/main.ino for an Arduino Uno using arduino-cli.
# arduino-cli wants the sketch folder name to match the .ino name, so we copy
# src/ into build/main/ first. The source files themselves stay untouched.
set -euo pipefail
cd "$(dirname "$0")/.."
rm -rf build/main
mkdir -p build/main
cp src/main.ino src/monitor_logic.h build/main/
arduino-cli compile --fqbn arduino:avr:uno --warnings default --output-dir build/out build/main "$@"
