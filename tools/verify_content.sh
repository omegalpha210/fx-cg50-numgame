#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
source tools/env.sh
export NUMGAME_VERIFY_OUTPUT="$PWD/build-host/content-verification"
mkdir -p "$NUMGAME_VERIFY_OUTPUT"
"$NUMGAME_PYTHON" tools/check_embedded.py
"$NUMGAME_PYTHON" tools/generate/guesscalc_verify.py
"$NUMGAME_PYTHON" tools/generate/guesscalc_verify_master.py
"$NUMGAME_PYTHON" tools/generate/guesscalc_beta4.py
"$NUMGAME_PYTHON" tools/generate/guesscalc_beta4_target.py --check
"$NUMGAME_PYTHON" tools/generate/guesscalc_beta4_target.py --verify --jobs 4
"$NUMGAME_PYTHON" tools/generate/guesscalc_beta4_audit.py --check
"$NUMGAME_PYTHON" tools/generate/guesscalc_beta5_target.py --check
"$NUMGAME_PYTHON" tools/generate/guesscalc_beta5_target.py --verify --jobs 4
"$NUMGAME_PYTHON" tools/generate/guesscalc_beta5_audit.py --check
"$NUMGAME_PYTHON" tools/generate/guesscalc_beta5_rows.py --check
"$NUMGAME_PYTHON" tools/generate/guesscalc_beta6_target.py --check
"$NUMGAME_PYTHON" tools/generate/guesscalc_beta6_refine.py --check
"$NUMGAME_PYTHON" tools/generate/guesscalc_beta6_target.py --verify --jobs 5
"$NUMGAME_PYTHON" tools/generate/guesscalc_beta6_countdown.py --verify
"$NUMGAME_PYTHON" tools/generate/guesscalc_beta6_audit.py
"$NUMGAME_PYTHON" tools/generate/guesscalc_beta6_native_audit.py --check
"$NUMGAME_PYTHON" tools/generate/guesscalc_prime_beta6.py --check
"$NUMGAME_PYTHON" tests/test_guesscalc_prime_beta6.py
"$NUMGAME_PYTHON" tests/test_guesscalc_beta6_content.py
"$NUMGAME_PYTHON" tools/generate/guesscalc_difficulty_audit.py --check > "$NUMGAME_VERIFY_OUTPUT/guesscalc-beta3-baseline.json"
"$NUMGAME_PYTHON" tests/test_guesscalc_exact.py
cmp assets/guesscalc_make_target_complexity.csv docs/MAKE_TARGET_COMPLEXITY.csv
cmp assets/guesscalc_target_beta5_complexity.csv docs/MAKE_TARGET_COMPLEXITY_BETA5.csv
"$NUMGAME_PYTHON" tests/test_guesscalc_cryptarithm_generate.py
"$NUMGAME_PYTHON" tools/generate/grids_verify.py
"$NUMGAME_PYTHON" tests/test_grids_master.py
"$NUMGAME_PYTHON" tests/test_grids_extra.py
"$NUMGAME_PYTHON" tools/generate/boards_verify.py
"$NUMGAME_PYTHON" tools/generate/strategyquick_master_verify.py
"$NUMGAME_PYTHON" tools/generate/boards_difficulty.py --verify
"$NUMGAME_PYTHON" tests/test_make_target_reference.py --all-fixed > "$NUMGAME_VERIFY_OUTPUT/make-target-reference.json"
"$NUMGAME_PYTHON" tests/test_make_target_reference.py --bank-sample20 --bank-revision 4 > "$NUMGAME_VERIFY_OUTPUT/make-target-beta4-sample20.json"
"$NUMGAME_PYTHON" tests/test_make_target_reference.py --bank-sample20 --bank-revision 5 > "$NUMGAME_VERIFY_OUTPUT/make-target-beta5-sample20.json"
"$NUMGAME_PYTHON" tests/test_grids_difficulty.py --output "$NUMGAME_VERIFY_OUTPUT/grids-difficulty.json"
cmp assets/grids/extra/difficulty-beta3.json "$NUMGAME_VERIFY_OUTPUT/grids-difficulty.json"
"$NUMGAME_PYTHON" tools/generate/difficulty_audit.py --check
# Runtime policies are compared with independent reference analyses here.
cmake -S tests -B build-host -DNG_SANITIZE=ON -DNG_ASAN=OFF
cmake --build build-host --target test_strategyquick test_strategyquick_difficulty test_strategyquick_sliding -j8
./build-host/test_strategyquick
"$NUMGAME_PYTHON" tools/generate/strategyquick_sliding.py --verify
"$NUMGAME_PYTHON" tools/generate/strategyquick_sliding_verify.py --sample-exe build-host/test_strategyquick_sliding --output "$NUMGAME_VERIFY_OUTPUT/sliding-beta4-audit.json"
cmp assets/strategyquick/sliding-beta4-audit.json "$NUMGAME_VERIFY_OUTPUT/sliding-beta4-audit.json"
"$NUMGAME_PYTHON" tools/generate/strategyquick_difficulty.py --sample-exe build-host/test_strategyquick_difficulty --output "$NUMGAME_VERIFY_OUTPUT/strategyquick-difficulty.json"
cmp assets/strategyquick/difficulty-beta4.json "$NUMGAME_VERIFY_OUTPUT/strategyquick-difficulty.json"
cmp assets/strategyquick/difficulty-beta4.csv "$NUMGAME_VERIFY_OUTPUT/strategyquick-difficulty.csv"
