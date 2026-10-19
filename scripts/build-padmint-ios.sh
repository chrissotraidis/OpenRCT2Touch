#!/usr/bin/env bash
# PadMint's iPad build of OpenRCT2 Touch, from a fresh clone: the pinned iOS
# dependencies, the engine data and the device app, packaged as an audited,
# unsigned IPA. No RollerCoaster Tycoon data is read: players import their own
# RCT2 or RCT Classic folder inside the app after installing it.
#
#   scripts/build-padmint-ios.sh OUTPUT.ipa
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
output="${1:?usage: scripts/build-padmint-ios.sh OUTPUT.ipa}"
cd "$root"

echo "OpenRCT2 Touch: building the pinned iOS dependencies"
./scripts/build-ios-deps.sh device
echo "OpenRCT2 Touch: building the engine data and the iPad app"
./scripts/build-ios.sh device
echo "OpenRCT2 Touch: packaging"
./scripts/package-ios-ipa.sh "$root/build/ios-libs-ios-arm64/OpenRCT2Touch.app" "$output"

