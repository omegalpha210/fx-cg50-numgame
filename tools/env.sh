#!/usr/bin/env bash
# Reuse an installed SDK. This script never writes into its directory.
NUMGAME_SDK_ROOT="${NUMGAME_SDK_ROOT:-$HOME/.local/diffeq-sdk}"
export PATH="$NUMGAME_SDK_ROOT/prefix/bin:$NUMGAME_SDK_ROOT/prefix/share/fxsdk/sysroot/bin:$NUMGAME_SDK_ROOT/venv/bin:$PATH"
if [[ -z "${NUMGAME_PYTHON:-}" ]]; then
  if [[ -x "$NUMGAME_SDK_ROOT/venv/bin/python3" ]]; then NUMGAME_PYTHON="$NUMGAME_SDK_ROOT/venv/bin/python3";
  else NUMGAME_PYTHON=python3; fi
fi
export NUMGAME_PYTHON
