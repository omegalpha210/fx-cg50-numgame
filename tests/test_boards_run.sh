#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build-boards
clang -std=c11 -Wall -Wextra -Werror -O1 -g -UNDEBUG \
 -fsanitize=undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
 -DBOARDS_TEST_MAIN -Iinclude -Isrc/games tests/test_boards.c tests/test_boards_registry.c \
 src/games/boards.c src/games/boards_pack.c src/core/common.c src/storage/codec.c src/ui/draw.c \
 -o build-boards/test_boards
./build-boards/test_boards
