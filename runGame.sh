#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/build/bin"
ls -l assets/shaders || true
./My3DEngine 2>err.log || true
echo "---------------- stderr ----------------"
tail -n +1 err.log