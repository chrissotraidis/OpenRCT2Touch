#!/usr/bin/env bash

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "$ROOT/scripts/env.sh"

INSTALL_DIR="$MACOS_BUILD_DIR/install"

"$ROOT/scripts/check-repo-safety.sh"
mkdir -p "$MACOS_BUILD_DIR" "$OPENRCT2_DATA"

cmake -S "$ROOT" -B "$MACOS_BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
    -DCMAKE_INSTALL_MESSAGE=LAZY \
    -DARCH="$(uname -m)" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
    -DMACOS_USE_DEPENDENCIES=ON \
    -DMACOS_BUNDLE=OFF \
    -DWITH_TESTS=OFF \
    -DDOWNLOAD_TITLE_SEQUENCES=ON \
    -DDOWNLOAD_OBJECTS=ON \
    -DDOWNLOAD_OPENSFX=ON \
    -DDOWNLOAD_OPENMUSIC=ON \
    "$@"

cmake --build "$MACOS_BUILD_DIR" --parallel
cmake -E rm -f "$INSTALL_DIR/bin/openrct2" "$INSTALL_DIR/bin/openrct2-cli"
cmake --install "$MACOS_BUILD_DIR"
cmake -E copy_directory "$INSTALL_DIR/share/openrct2" "$OPENRCT2_DATA"

test -x "$INSTALL_DIR/bin/openrct2"
test -f "$OPENRCT2_DATA/g2.dat"
test -d "$OPENRCT2_DATA/language"

printf 'macOS build passed.\n'
printf '  binary: %s\n' "$INSTALL_DIR/bin/openrct2"
printf '  engine data: %s\n' "$OPENRCT2_DATA"
