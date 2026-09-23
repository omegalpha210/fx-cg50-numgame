#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
source tools/env.sh
cmake -S tests -B build-host -DCMAKE_C_COMPILER=clang -DNG_SANITIZE=ON
cmake --build build-host -j8
ctest --test-dir build-host --output-on-failure
"$NUMGAME_PYTHON" tools/check_embedded.py
