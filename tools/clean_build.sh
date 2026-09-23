#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
source tools/env.sh
NUMGAME_VARIANT=OFF
NUMGAME_ARTIFACT=NUMGAME.g3a
if [[ "${1:-}" == --diagnostic ]]; then NUMGAME_VARIANT=ON; NUMGAME_ARTIFACT=NUMGDIAG.g3a
elif [[ $# != 0 ]]; then printf 'Usage: %s [--diagnostic]\n' "$0" >&2; exit 2; fi
NUMGAME_CLEAN_BUILD="$(mktemp -d "$PWD/build-clean-XXXXXX")"
cmake -S . -B "$NUMGAME_CLEAN_BUILD" -DNG_DIAGNOSTIC="$NUMGAME_VARIANT" \
  -DCMAKE_MODULE_PATH="$NUMGAME_SDK_ROOT/prefix/lib/cmake/fxsdk" \
  -DFXSDK_CMAKE_MODULE_PATH="$NUMGAME_SDK_ROOT/prefix/lib/cmake/fxsdk" \
  -DCMAKE_TOOLCHAIN_FILE="$NUMGAME_SDK_ROOT/prefix/lib/cmake/fxsdk/FXCG50.cmake"
cmake --build "$NUMGAME_CLEAN_BUILD" -j8
"$NUMGAME_PYTHON" tools/verify_g3a.py "dist/$NUMGAME_ARTIFACT"
(cd dist && shasum -a 256 ./*.g3a > SHA256SUMS.txt)
printf 'Fresh native build retained at %s\n' "$NUMGAME_CLEAN_BUILD"
