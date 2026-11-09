#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/env.sh"

search_regex() {
    if command -v rg >/dev/null 2>&1; then
        rg -n -- "$@"
    else
        grep -R -n -E -- "$@"
    fi
}

search_fixed() {
    if command -v rg >/dev/null 2>&1; then
        rg -n -F -- "$@"
    else
        grep -R -n -F -- "$@"
    fi
}

"$ROOT/scripts/check-repo-safety.sh"

for script in \
    scripts/bootstrap.sh \
    scripts/build-ios.sh \
    scripts/build-ios-device.sh \
    scripts/collect-crash.sh \
    scripts/compile-ios-assets.sh \
    scripts/install-run-ios.sh \
    scripts/package-ios-ipa.sh \
    scripts/run-ios-sim.sh \
    scripts/seed-ios-sim-data.sh \
    scripts/stage-ios-licenses.sh \
    scripts/verify-ios-bundle.sh \
    scripts/verify-ios-sandbox.sh; do
    bash -n "$ROOT/$script"
done

if [[ "$(plutil -extract CFBundleIdentifier raw "$ROOT/ios/App/Info.plist")" != '${MACOSX_BUNDLE_GUI_IDENTIFIER}' ]]; then
    echo "Info.plist must use the configured CMake bundle identifier." >&2
    exit 1
fi

if search_regex 'org\.openrct2\.touch' "$ROOT/ios" "$ROOT/scripts" >/dev/null; then
    echo "The deprecated OpenRCT2-owned bundle namespace remains in build code." >&2
    exit 1
fi

for required_file in \
    NOTICE.md \
    licence.txt \
    contributors.md \
    vendor/MANIFEST.md \
    docs/RELEASE-CHECKLIST.md; do
    if [[ ! -f "$ROOT/$required_file" ]]; then
        echo "Missing release document: $required_file" >&2
        exit 1
    fi
done

for forbidden_claim in \
    'with Apple Pencil support' \
    'eventually Apple Pencil' \
    'Phase 8 — Apple Pencil' \
    'Goal 8 — Apple Pencil'; do
    if search_fixed "$forbidden_claim" \
        "$ROOT/AGENTS.md" \
        "$ROOT/GOAL-LOOP.md" \
        "$ROOT/readme.md" \
        "$ROOT/docs" >/dev/null; then
        echo "Unsupported public claim remains: $forbidden_claim" >&2
        exit 1
    fi
done

if ! search_fixed 'Apple Pencil is not currently supported' "$ROOT/readme.md" >/dev/null; then
    echo "The canonical README must state the Apple Pencil limitation." >&2
    exit 1
fi
if ! search_fixed 'Networking is disabled in the current iPadOS build' "$ROOT/readme.md" >/dev/null; then
    echo "The canonical README must state the multiplayer limitation." >&2
    exit 1
fi
if search_fixed 'OPENRCT2_SKIP_MACOS_BUILD=1' \
    "$ROOT/readme.md" \
    "$ROOT/docs/DEVELOPMENT-STATUS.md" >/dev/null; then
    echo "Public install guidance must generate redistributable engine assets before the device build." >&2
    exit 1
fi
if search_fixed '.ipa' "$ROOT/.github/workflows" >/dev/null; then
    echo "GitHub workflows must not package an IPA while OpenRCT2 Touch remains a source-only developer preview." >&2
    exit 1
fi

echo "OpenRCT2 Touch release-safety checks passed."
