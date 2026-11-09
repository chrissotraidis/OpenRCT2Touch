#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/env.sh"

MODE="${1:-unsigned}"
case "$MODE" in
    signed|unsigned) ;;
    *)
        echo "Usage: $0 [unsigned|signed]" >&2
        exit 2
        ;;
esac

TEAM="${OPENRCT2_DEVELOPMENT_TEAM:-}"
DEVICE="${OPENRCT2_DEVICE_UDID:-}"
if [[ "$MODE" == "signed" && -z "$TEAM" ]]; then
    echo "Set OPENRCT2_DEVELOPMENT_TEAM to the Apple team identifier selected in Xcode." >&2
    echo "The team identifier is intentionally never inferred or written to the repository." >&2
    exit 2
fi

"$ROOT/scripts/check-repo-safety.sh"
if [[ "${OPENRCT2_SKIP_MACOS_BUILD:-0}" != "1" ]]; then
    "$ROOT/scripts/build-macos.sh"
fi

missing_engine_assets=()
for required_asset in g2.dat fonts.dat palettes.dat tracks.dat language/en-GB.txt; do
    if [[ ! -f "$ROOT/assets/engine/$required_asset" ]]; then
        missing_engine_assets+=("$required_asset")
    fi
done
if [[ "${#missing_engine_assets[@]}" -ne 0 ]]; then
    printf 'Missing redistributable engine assets:' >&2
    printf ' %s' "${missing_engine_assets[@]}" >&2
    printf '\n' >&2
    if [[ "${OPENRCT2_SKIP_MACOS_BUILD:-0}" == "1" ]]; then
        echo "OPENRCT2_SKIP_MACOS_BUILD=1 prevented generation of these required OpenRCT2 files." >&2
        echo "They are redistributable app components, not user-owned RCT2 game data." >&2
        echo "RCT2 game data is not required to build the iPad app." >&2
        echo "Run 'unset OPENRCT2_SKIP_MACOS_BUILD', then rerun your complete build command without the skip flag." >&2
        echo "Do not run the install command until a signed build reports that it passed." >&2
    else
        echo "The macOS build did not generate the required iOS engine assets." >&2
    fi
    exit 1
fi

VCPKG_ROOT="$ROOT/vendor/vcpkg"
INSTALL_ROOT="$ROOT/vendor/ios-arm64"
TRIPLET="openrct2-arm64-ios"
BUILD_ROOT="$ROOT/build/ios-xcode-device"
DERIVED_DATA="$BUILD_ROOT/DerivedData"
PROJECT="$BUILD_ROOT/OpenRCT2.xcodeproj"
APP="$BUILD_ROOT/Release-iphoneos/OpenRCT2Touch.app"

if [[ ! -x "$VCPKG_ROOT/vcpkg" || ! -d "$INSTALL_ROOT/$TRIPLET" ]]; then
    echo "Missing pinned device dependencies. Run ./scripts/build-ios-deps.sh device first." >&2
    exit 1
fi

SIGNING_ENABLED=OFF
SIGNING_ALLOWED=NO
if [[ "$MODE" == "signed" ]]; then
    SIGNING_ENABLED=ON
    SIGNING_ALLOWED=YES
fi

cmake -S "$ROOT" -B "$BUILD_ROOT" -G Xcode \
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
    -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE="$ROOT/ios/toolchain/ios.toolchain.cmake" \
    -DOPENRCT2_IOS_PLATFORM=DEVICE \
    -DOPENRCT2_IOS_CODE_SIGNING="$SIGNING_ENABLED" \
    -DOPENRCT2_IOS_DEVELOPMENT_TEAM="$TEAM" \
    -DOPENRCT2_IOS_BUNDLE_ID="$OPENRCT2_TOUCH_BUNDLE_ID" \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 \
    -DVCPKG_TARGET_TRIPLET="$TRIPLET" \
    -DVCPKG_INSTALLED_DIR="$INSTALL_ROOT" \
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

DESTINATION="generic/platform=iOS"
if [[ -n "$DEVICE" ]]; then
    DESTINATION="id=$DEVICE"
fi

# Xcode can leave a signed bundle from a previous device run in the shared
# output directory. An unsigned rebuild must never inherit stale signatures or
# package contents, so recreate only the generated app bundle before building.
cmake -E remove_directory "$APP"

XCODE_ARGS=(
    -project "$PROJECT"
    -scheme openrct2-touch
    -configuration Release
    -sdk iphoneos
    -destination "$DESTINATION"
    -derivedDataPath "$DERIVED_DATA"
    CODE_SIGNING_ALLOWED="$SIGNING_ALLOWED"
    CLANG_WARN_64_TO_32_BIT_CONVERSION=NO
    WARNING_CFLAGS=-Wno-shorten-64-to-32
)
if [[ "$MODE" == "signed" ]]; then
    XCODE_ARGS+=(
        CODE_SIGNING_REQUIRED=YES
        CODE_SIGN_STYLE=Automatic
        DEVELOPMENT_TEAM="$TEAM"
        -allowProvisioningUpdates
        -allowProvisioningDeviceRegistration
    )
fi

xcodebuild "${XCODE_ARGS[@]}" build
"$ROOT/scripts/verify-ios-bundle.sh" "$APP" IOS

if [[ "$MODE" == "signed" ]]; then
    test -f "$APP/embedded.mobileprovision"
    test -f "$APP/_CodeSignature/CodeResources"
fi

echo "Xcode iPadOS $MODE build passed."
echo "  app: $APP"
if [[ "$MODE" == "signed" ]]; then
    echo "  install: OPENRCT2_DEVICE_UDID=<device> ./scripts/install-run-ios.sh"
fi
