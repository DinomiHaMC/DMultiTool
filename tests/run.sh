#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
binary="$(mktemp /tmp/hp2000-keyboard.XXXXXX)"
trap 'rm -f "$binary"' EXIT
g++ -std=c++17 -Wall -Wextra -Werror -Itests/stubs src/input/Keyboard.cpp tests/audio_control_test.cpp -o "$binary"
"$binary"
g++ -std=c++17 -Wall -Wextra -Werror tests/radio_memory_test.cpp -o "$binary"
"$binary"
g++ -std=c++17 -Wall -Wextra -Werror src/services/ClassicNDEF.cpp src/services/NDEFCodec.cpp tests/classic_ndef_test.cpp -o "$binary"
"$binary"
g++ -std=c++17 -Wall -Wextra -Werror tests/nfc_polling_test.cpp -o "$binary"
"$binary"
g++ -std=c++17 -Wall -Wextra -Werror src/games/GameModels.cpp tests/games_test.cpp -o "$binary"
"$binary"
g++ -std=c++17 -Wall -Wextra -Werror -Itests/stubs src/input/Keyboard.cpp tests/keyboard_test.cpp -o "$binary"
"$binary"

g++ -std=c++17 -Wall -Wextra -Werror src/services/NDEFCodec.cpp src/services/ScriptParser.cpp tests/codec_test.cpp -o "$binary"
"$binary"

g++ -std=c++17 -Wall -Wextra -Wno-missing-field-initializers -Itests/display-stubs src/ui/KeyboardModel.cpp src/ui/DisplayManager.cpp src/ui/Theme.cpp tests/display_test.cpp -o "$binary"
"$binary"
g++ -std=c++17 -Wall -Wextra -Wno-missing-field-initializers -Itests/display-stubs src/ui/KeyboardModel.cpp src/ui/DisplayManager.cpp src/ui/Theme.cpp src/ui/UI.cpp tests/ui_test.cpp -o "$binary"
"$binary"
