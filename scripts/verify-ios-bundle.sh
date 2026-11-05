#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/env.sh"

APP="${1:-}"
PLATFORM="${2:-}"
GREP=/usr/bin/grep

if [[ -z "$APP" || ! -d "$APP" || ( "$PLATFORM" != "IOS" && "$PLATFORM" != "IOSSIMULATOR" ) ]]; then
    echo "Usage: $0 <OpenRCT2Touch.app> <IOS|IOSSIMULATOR>" >&2
    exit 2
fi
if [[ ! -x "$GREP" ]]; then
    echo "Required macOS system tool is unavailable: $GREP" >&2
    exit 1
fi

"$ROOT/scripts/check-repo-safety.sh"

BINARY="$APP/OpenRCT2Touch"
plutil -lint "$APP/Info.plist"
if [[ "$(plutil -extract CFBundleIdentifier raw "$APP/Info.plist")" != "$OPENRCT2_TOUCH_BUNDLE_ID" ]]; then
    echo "Unexpected iOS bundle identifier." >&2
    exit 1
fi
if [[ "$(plutil -extract 'UIApplicationSceneManifest.UIApplicationSupportsMultipleScenes' raw "$APP/Info.plist")" != "false"
    || "$(plutil -extract 'UIApplicationSceneManifest.UISceneConfigurations.UIWindowSceneSessionRoleApplication.0.UISceneDelegateClassName' raw "$APP/Info.plist")" != "SDLUIKitSceneDelegate" ]]; then
    echo "App bundle does not declare the required single-window UIScene lifecycle." >&2
    exit 1
fi

for required_asset in g2.dat fonts.dat palettes.dat tracks.dat language/en-GB.txt; do
    if [[ ! -f "$APP/$required_asset" ]]; then
        echo "App bundle is missing engine asset: $required_asset" >&2
        exit 1
    fi
done

for required_notice in \
    Licences/GPL-3.0-or-later.txt \
    Licences/OpenRCT2-contributors.md \
    Licences/OpenRCT2-Touch-NOTICE.md \
    Licences/iOS-dependency-manifest.md; do
    if [[ ! -f "$APP/$required_notice" ]]; then
        echo "App bundle is missing distribution notice: $required_notice" >&2
        exit 1
    fi
done

for dependency in sdl2 icu freetype libpng zlib zstd libzip nlohmann-json; do
    if [[ ! -f "$APP/Licences/third-party/$dependency.txt" ]]; then
        echo "App bundle is missing third-party licence text: $dependency" >&2
        exit 1
    fi
done

if [[ "$(plutil -extract 'CFBundleIcons.CFBundlePrimaryIcon.CFBundleIconName' raw "$APP/Info.plist")" != "AppIcon"
    || "$(plutil -extract 'CFBundleIcons~ipad.CFBundlePrimaryIcon.CFBundleIconName' raw "$APP/Info.plist")" != "AppIcon" ]]; then
    echo "App bundle does not declare the compiled AppIcon catalog." >&2
    exit 1
fi

if [[ "$(plutil -extract UIDeviceFamily.0 raw "$APP/Info.plist")" != "1"
    || "$(plutil -extract UIDeviceFamily.1 raw "$APP/Info.plist")" != "2" ]]; then
    echo "App bundle must target both iPhone and iPad." >&2
    exit 1
fi

for orientation_key in UISupportedInterfaceOrientations 'UISupportedInterfaceOrientations~ipad'; do
    for orientation_index in 0 1; do
        expected=UIInterfaceOrientationLandscapeLeft
        if [[ "$orientation_index" == "1" ]]; then
            expected=UIInterfaceOrientationLandscapeRight
        fi
        if [[ "$(plutil -extract "$orientation_key.$orientation_index" raw "$APP/Info.plist")" != "$expected" ]]; then
            echo "App bundle must support both landscape orientations on iPhone and iPad." >&2
            exit 1
        fi
    done
done

for icon_spec in "AppIcon60x60@2x.png:120" "AppIcon76x76@2x~ipad.png:152"; do
    IFS=: read -r icon_name expected_size <<< "$icon_spec"
    icon_path="$APP/$icon_name"
    if [[ ! -f "$icon_path" ]]; then
        echo "App bundle is missing compiled icon: $icon_name" >&2
        exit 1
    fi
    icon_alpha="$(sips -g hasAlpha "$icon_path" | awk '/hasAlpha:/ { print $2 }')"
    if [[ "$(sips -g pixelWidth "$icon_path" | awk '/pixelWidth:/ { print $2 }')" != "$expected_size"
        || "$(sips -g pixelHeight "$icon_path" | awk '/pixelHeight:/ { print $2 }')" != "$expected_size"
        || ( "$PLATFORM" == "IOS" && "$icon_alpha" != "no" ) ]]; then
        echo "Compiled icon has unexpected dimensions or device transparency: $icon_name" >&2
        exit 1
    fi
done

if [[ ! -f "$APP/Assets.car" ]]; then
    echo "App bundle is missing the compiled asset catalog." >&2
    exit 1
fi
ASSET_CATALOG_INFO="$(xcrun assetutil --info "$APP/Assets.car")"
if [[ "$(printf '%s' "$ASSET_CATALOG_INFO" | plutil -extract 1.Name raw -)" != "AppIcon"
    || "$(printf '%s' "$ASSET_CATALOG_INFO" | plutil -extract 1.AssetType raw -)" != "Icon Image"
    || "$(printf '%s' "$ASSET_CATALOG_INFO" | plutil -extract 1.Opaque raw -)" != "true"
    || "$(printf '%s' "$ASSET_CATALOG_INFO" | plutil -extract 1.PixelWidth raw -)" != "1024"
    || "$(printf '%s' "$ASSET_CATALOG_INFO" | plutil -extract 1.PixelHeight raw -)" != "1024" ]]; then
    echo "Compiled asset catalog does not contain the expected opaque 1024px AppIcon rendition." >&2
    exit 1
fi

PROPRIETARY_MATCHES=""
if PROPRIETARY_MATCHES="$(
    find "$APP" -type f -print \
        | "$GREP" -Ei '(^|/)(g1\.dat|css1\.dat|css2\.dat|rct2\.exe)$|/(ObjData|Scenarios)/|\.(sv4|sv6|sc4|sc6|td4|td6)$'
)"; then
    :
else
    proprietary_scan_status=$?
    if [[ "$proprietary_scan_status" -ne 1 ]]; then
        echo "Unable to audit the app bundle for proprietary game data." >&2
        exit 1
    fi
fi
if [[ -n "$PROPRIETARY_MATCHES" ]]; then
    echo "Proprietary game-data signature found in app bundle:" >&2
    echo "$PROPRIETARY_MATCHES" >&2
    exit 1
fi

UNEXPECTED_LINKS="$(find "$APP" -type l -print)"
if [[ -n "$UNEXPECTED_LINKS" ]]; then
    echo "Unexpected symbolic link found in app bundle:" >&2
    echo "$UNEXPECTED_LINKS" >&2
    exit 1
fi

while IFS= read -r -d '' bundled_file; do
    relative_path="${bundled_file#"$APP"/}"
    case "$relative_path" in
        Info.plist|OpenRCT2Touch|embedded.mobileprovision|_CodeSignature/CodeResources|Assets.car|AppIcon60x60@2x.png|AppIcon76x76@2x~ipad.png)
            continue
            ;;
        Licences/GPL-3.0-or-later.txt)
            cmp -s "$ROOT/licence.txt" "$bundled_file" || { echo "Bundled GPL text differs from licence.txt." >&2; exit 1; }
            continue
            ;;
        Licences/OpenRCT2-contributors.md)
            cmp -s "$ROOT/contributors.md" "$bundled_file" || { echo "Bundled contributors file differs from contributors.md." >&2; exit 1; }
            continue
            ;;
        Licences/OpenRCT2-Touch-NOTICE.md)
            cmp -s "$ROOT/NOTICE.md" "$bundled_file" || { echo "Bundled notice differs from NOTICE.md." >&2; exit 1; }
            continue
            ;;
        Licences/iOS-dependency-manifest.md)
            cmp -s "$ROOT/vendor/MANIFEST.md" "$bundled_file" || { echo "Bundled dependency manifest differs from vendor/MANIFEST.md." >&2; exit 1; }
            continue
            ;;
        Licences/third-party/*.txt)
            dependency="${relative_path##*/}"
            dependency="${dependency%.txt}"
            if [[ "$PLATFORM" == "IOS" ]]; then
                licence_source="$ROOT/vendor/ios-arm64/openrct2-arm64-ios/share/$dependency/copyright"
            else
                licence_source="$ROOT/vendor/ios-sim-arm64/openrct2-arm64-ios-simulator/share/$dependency/copyright"
            fi
            if [[ ! -f "$licence_source" ]] || ! cmp -s "$licence_source" "$bundled_file"; then
                echo "Bundled third-party licence is missing or altered: $dependency" >&2
                exit 1
            fi
            continue
            ;;
        PkgInfo)
            if ! cmp -s <(printf 'APPL????') "$bundled_file"; then
                echo "Unexpected iOS PkgInfo contents." >&2
                exit 1
            fi
            continue
            ;;
    esac

    engine_source="$ROOT/assets/engine/$relative_path"
    if [[ ! -f "$engine_source" ]]; then
        echo "App bundle contains a file outside the redistributable engine manifest: $relative_path" >&2
        exit 1
    fi
    if ! cmp -s "$engine_source" "$bundled_file"; then
        echo "Bundled engine asset differs from its redistributable source: $relative_path" >&2
        exit 1
    fi
done < <(find "$APP" -type f -print0)

file "$BINARY"
if ! nm "$BINARY" | "$GREP" -F '_OBJC_CLASS_$_SDLUIKitSceneDelegate' >/dev/null; then
    echo "App binary is missing the SDL UIKit scene delegate required by iPadOS 27." >&2
    exit 1
fi
if ! BUILD_VERSION="$(xcrun vtool -show-build "$BINARY")"; then
    echo "Unable to inspect the app binary's Apple platform identity." >&2
    exit 1
fi
printf '%s\n' "$BUILD_VERSION"
if ! printf '%s\n' "$BUILD_VERSION" | "$GREP" -F "platform $PLATFORM" >/dev/null; then
    echo "App binary has the wrong Apple platform identity; expected $PLATFORM." >&2
    exit 1
fi

if [[ -d "$APP/_CodeSignature" ]]; then
    codesign --verify --deep --strict --verbose=2 "$APP"
fi

echo "iOS app bundle passed for $PLATFORM: $APP"
