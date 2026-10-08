#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
source tools/env.sh
NUMGAME_VARIANT=OFF
NUMGAME_ARTIFACT=NUMGAME.g3a
NUMGAME_BUILD_KIND=normal
if [[ $# == 1 && "$1" == --diagnostic ]]; then
  NUMGAME_VARIANT=ON; NUMGAME_ARTIFACT=NUMGDIAG.g3a; NUMGAME_BUILD_KIND=diagnostic
elif [[ $# != 0 ]]; then
  printf 'Usage: %s [--diagnostic]\n' "$0" >&2; exit 2
fi
NUMGAME_REPO_ROOT="$(pwd -P)"
NUMGAME_CLEAN_BUILD="$NUMGAME_REPO_ROOT/build/clean/$NUMGAME_BUILD_KIND"
case "$NUMGAME_CLEAN_BUILD" in
  "$NUMGAME_REPO_ROOT/build/clean/normal"|"$NUMGAME_REPO_ROOT/build/clean/diagnostic") ;;
  *) printf 'Refusing unsafe build path: %s\n' "$NUMGAME_CLEAN_BUILD" >&2; exit 2 ;;
esac
for parent in "$NUMGAME_REPO_ROOT/build" "$NUMGAME_REPO_ROOT/build/clean" "$NUMGAME_CLEAN_BUILD"; do
  if [[ -L "$parent" ]]; then printf 'Refusing symlinked build path: %s\n' "$parent" >&2; exit 2; fi
done
if [[ -e "$NUMGAME_CLEAN_BUILD" ]]; then
  if [[ ! -f "$NUMGAME_CLEAN_BUILD/.numgame-clean-build" ]]; then
    printf 'Refusing to remove unmarked build directory: %s\n' "$NUMGAME_CLEAN_BUILD" >&2
    exit 2
  fi
  rm -rf -- "$NUMGAME_CLEAN_BUILD"
fi
mkdir -p "$NUMGAME_CLEAN_BUILD"
printf '%s\n' 'NUM GAME generated native build directory' > "$NUMGAME_CLEAN_BUILD/.numgame-clean-build"
cmake -S . -B "$NUMGAME_CLEAN_BUILD" -DNG_DIAGNOSTIC="$NUMGAME_VARIANT" \
  -DCMAKE_MODULE_PATH="$NUMGAME_SDK_ROOT/prefix/lib/cmake/fxsdk" \
  -DFXSDK_CMAKE_MODULE_PATH="$NUMGAME_SDK_ROOT/prefix/lib/cmake/fxsdk" \
  -DCMAKE_TOOLCHAIN_FILE="$NUMGAME_SDK_ROOT/prefix/lib/cmake/fxsdk/FXCG50.cmake"
cmake --build "$NUMGAME_CLEAN_BUILD" -j8
"$NUMGAME_PYTHON" tools/verify_g3a.py "dist/$NUMGAME_ARTIFACT"
(cd dist && shasum -a 256 ./*.g3a > SHA256SUMS.txt)
printf 'Fresh native build retained at %s\n' "$NUMGAME_CLEAN_BUILD"
