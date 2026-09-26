#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
source tools/env.sh
export NUMGAME_VERIFY_OUTPUT="$PWD/build-host/content-verification"
mkdir -p "$NUMGAME_VERIFY_OUTPUT"
"$NUMGAME_PYTHON" tools/check_embedded.py
"$NUMGAME_PYTHON" tools/generate/guesscalc_verify.py
"$NUMGAME_PYTHON" tools/generate/guesscalc_verify_master.py
"$NUMGAME_PYTHON" tests/test_guesscalc_cryptarithm_generate.py
"$NUMGAME_PYTHON" tools/generate/grids_verify.py
"$NUMGAME_PYTHON" tests/test_grids_master.py
"$NUMGAME_PYTHON" tests/test_grids_extra.py
"$NUMGAME_PYTHON" tools/generate/boards_verify.py
"$NUMGAME_PYTHON" tools/generate/strategyquick_master_verify.py
"$NUMGAME_PYTHON" tools/generate/boards_difficulty.py --verify
"$NUMGAME_PYTHON" tools/generate/guesscalc_difficulty_audit.py --check > "$NUMGAME_VERIFY_OUTPUT/guesscalc-difficulty.json"
"$NUMGAME_PYTHON" tests/test_grids_difficulty.py --output "$NUMGAME_VERIFY_OUTPUT/grids-difficulty.json"
cmp assets/grids/extra/difficulty-beta3.json "$NUMGAME_VERIFY_OUTPUT/grids-difficulty.json"
"$NUMGAME_PYTHON" tools/generate/difficulty_audit.py --check
# Runtime policies are compared with independent reference analyses here.
cmake -S tests -B build-host -DNG_SANITIZE=ON -DNG_ASAN=OFF
cmake --build build-host --target test_strategyquick test_strategyquick_difficulty -j8
./build-host/test_strategyquick
"$NUMGAME_PYTHON" tools/generate/strategyquick_difficulty.py --sample-exe build-host/test_strategyquick_difficulty --output "$NUMGAME_VERIFY_OUTPUT/strategyquick-difficulty.json"
cmp assets/strategyquick/difficulty-beta3.json "$NUMGAME_VERIFY_OUTPUT/strategyquick-difficulty.json"
