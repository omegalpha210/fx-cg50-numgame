#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
source tools/env.sh
export NUMGAME_VERIFY_OUTPUT="$PWD/build-host/content-verification"
mkdir -p "$NUMGAME_VERIFY_OUTPUT"
"$NUMGAME_PYTHON" tools/check_embedded.py
"$NUMGAME_PYTHON" tools/generate/guesscalc_verify.py
"$NUMGAME_PYTHON" tools/generate/guesscalc_verify_master.py
"$NUMGAME_PYTHON" tools/generate/grids_verify.py
"$NUMGAME_PYTHON" tests/test_grids_master.py
"$NUMGAME_PYTHON" tools/generate/boards_verify.py
"$NUMGAME_PYTHON" tools/generate/strategyquick_master_verify.py
# Runtime policies are compared with independent reference analyses here.
cmake -S tests -B build-host -DNG_SANITIZE=ON -DNG_ASAN=OFF
cmake --build build-host --target test_strategyquick -j8
./build-host/test_strategyquick
