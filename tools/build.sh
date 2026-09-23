#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
source tools/env.sh
fxsdk build-cg -j8
"$NUMGAME_PYTHON" tools/verify_g3a.py dist/NUMGAME.g3a
(cd dist && shasum -a 256 ./*.g3a > SHA256SUMS.txt)
