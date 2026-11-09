#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/env.sh"

MODE="${1:-all}"
VCPKG_ROOT="$ROOT/vendor/vcpkg"

case "$MODE" in
    all|device|sim) ;;
    *)
        echo "Usage: $0 [all|device|sim]" >&2
        exit 2
        ;;
esac

"$ROOT/scripts/check-repo-safety.sh"

if [[ ! -x "$VCPKG_ROOT/vcpkg" ]]; then
    echo "Missing pinned dependencies. Run ./scripts/build-ios-deps.sh all first." >&2
    exit 1
fi

build_slice() {
    local label="$1"
    local triplet="$2"
    local platform="$3"
    local install_root="$ROOT/vendor/$label"
    local build_root="$ROOT/build/ios-libs-$label"

    if [[ ! -d "$install_root/$triplet" ]]; then
        echo "Missing dependency slice: $install_root/$triplet" >&2
        exit 1
    fi

    cmake -S "$ROOT" -B "$build_root" -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
        -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE="$ROOT/ios/toolchain/ios.toolchain.cmake" \
        -DOPENRCT2_IOS_PLATFORM="$platform" \
        -DOPENRCT2_IOS_BUNDLE_ID="$OPENRCT2_TOUCH_BUNDLE_ID" \
        -DCMAKE_OSX_ARCHITECTURES=arm64 \
        -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 \
        -DVCPKG_TARGET_TRIPLET="$triplet" \
        -DVCPKG_INSTALLED_DIR="$install_root" \
        -DVCPKG_MANIFEST_MODE=OFF \
        -DDISABLE_DISCORD_RPC=ON \
        -DDISABLE_HTTP=ON \
        -DDISABLE_NETWORK=ON \
        -DDISABLE_FLAC=ON \
        -DDISABLE_VORBIS=ON \
        -DDISABLE_OPENGL=ON \
        -DENABLE_SCRIPTING=ON \
        -DDISABLE_IPO=ON \
        -DWITH_TESTS=OFF \
        -DDOWNLOAD_TITLE_SEQUENCES=OFF \
        -DDOWNLOAD_OBJECTS=OFF \
        -DDOWNLOAD_OPENSFX=OFF \
        -DDOWNLOAD_OPENMUSIC=OFF

    cmake --build "$build_root" --target libopenrct2 openrct2-ui

    local engine="$build_root/libopenrct2.a"
    local ui="$build_root/libopenrct2-ui.a"
    lipo -info "$engine"
    lipo -info "$ui"

    if ! nm -gU "$ui" | grep ' T _SDL_main$' >/dev/null; then
        echo "The iOS UI archive does not export SDL_main." >&2
        exit 1
    fi
    if grep -E -- '-framework (Cocoa|CoreServices|ApplicationServices)' "$build_root/build.ninja"; then
        echo "A macOS-only framework leaked into the iOS target graph." >&2
        exit 1
    fi

    echo "iOS engine/UI libraries passed for: $platform"
}

if [[ "$MODE" == "all" || "$MODE" == "device" ]]; then
    build_slice ios-arm64 openrct2-arm64-ios DEVICE
fi
if [[ "$MODE" == "all" || "$MODE" == "sim" ]]; then
    build_slice ios-sim-arm64 openrct2-arm64-ios-simulator SIMULATOR
fi
