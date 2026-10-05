#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
commit="$(git rev-parse --short HEAD 2>/dev/null || true)"
if [[ ! "$commit" =~ ^[0-9a-f]+$ ]]; then commit=unversioned; fi
arduino-cli compile --fqbn esp32:esp32:esp32 --board-options PartitionScheme=huge_app --build-property "compiler.cpp.extra_flags=-DFW_COMMIT=\"$commit\"" "$@" .
