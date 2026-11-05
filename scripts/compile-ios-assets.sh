#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/env.sh"

APP="${1:-}"
TARGET="${2:-}"
case "$TARGET" in
    DEVICE) platform=iphoneos ;;
    SIMULATOR) platform=iphonesimulator ;;
    *)
        echo "Usage: $0 <app-bundle> <DEVICE|SIMULATOR>" >&2
        exit 2
        ;;
esac

if [[ ! -d "$APP" ]]; then
    echo "Missing app bundle: $APP" >&2
    exit 1
fi

"$ROOT/scripts/check-repo-safety.sh"
PARTIAL_PLIST="${APP%/*}/OpenRCT2Touch-assetcatalog-generated.plist"
xcrun actool "$ROOT/ios/App/Assets.xcassets" \
    --compile "$APP" \
    --output-format human-readable-text \
    --notices \
    --warnings \
    --output-partial-info-plist "$PARTIAL_PLIST" \
    --app-icon AppIcon \
    --compress-pngs \
    --development-region en \
    --target-device iphone \
    --target-device ipad \
    --minimum-deployment-target 15.0 \
    --platform "$platform"

echo "Compiled AppIcon assets for $TARGET in $APP"
