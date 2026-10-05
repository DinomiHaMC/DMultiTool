#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
binary="$(mktemp /tmp/hp2000-python.XXXXXX)"
trap 'rm -f "$binary"' EXIT
gcc -std=gnu99 -Os -ffunction-sections -fdata-sections -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast -Isrc/third_party/pikapython -Isrc/python src/third_party/pikapython/*.c src/python/PythonEngine.c tests/python_test.c -Wl,--gc-sections -lm -o "$binary"
timeout 30 "$binary"
