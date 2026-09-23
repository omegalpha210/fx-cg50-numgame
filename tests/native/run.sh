#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
mkdir -p build-host
sources=()
while IFS= read -r path; do
  if [[ "$path" != src/main.c && "$path" != src/storage/native.c ]]; then sources+=("$path"); fi
done < <(rg --files src -g '*.c')
for variant in release diagnostic; do
  flags=()
  if [[ "$variant" == diagnostic ]]; then flags+=(-DNG_DIAGNOSTIC -finstrument-functions); fi
  clang -std=c11 -Wall -Wextra -Werror -O1 -g -UNDEBUG \
    -fsanitize=undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
    -DFXCG50 "${flags[@]}" -Itests/native -Iinclude -Isrc/games \
    tests/test_native.c src/storage/native.c "${sources[@]}" -o "build-host/test_native_$variant"
  printf 'Native namespace variant: %s\n' "$variant"
  "./build-host/test_native_$variant"
done
