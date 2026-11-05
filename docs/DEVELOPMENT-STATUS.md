# OpenRCT2 Touch development status

- **Last updated:** November 5, 2026
- **Working branch:** `ipad`
- **Current goal:** [Goal 6 — Pointer, keyboard, and mouse play](../GOAL-LOOP.md#goal-6--pointer-keyboard-and-mouse-play)
- **Accepted checkpoint:** physical Files import, persistence, and scenario load on July 16

The implementation has completed Goals 0–5 and Goal 8. A signed build runs on
an iPad Pro (12.9-inch, 6th generation), imports user-owned RCT2 data through
Files, persists it across relaunches, loads scenarios, and has working pointer,
keyboard, trackpad, and finger controls. The human tester reports that the game
loads cleanly, appears stable, and works without an observed issue at this
checkpoint. Goals 6 and 7 have useful on-device progress, but their complete
end-to-end exit scripts are still pending.

## Experimental iPhone compatibility

The iOS bundle targets both iPhone and iPad with landscape orientation and
icons for both device families. **iPhone support is experimental**; physical
iPhone import, gameplay, layout, and performance validation remain pending.
iPad remains the primary tested platform and the MVP acceptance target. This
compatibility change does not close any device gate.

The November 5 PR #15 validation passed repository and release-safety checks,
the full macOS build and 1,000-tick headless smoke test (checksum
`25232284e49cf2cb000000000000000000000000`), Ninja iOS device and Simulator
builds, and the unsigned Xcode device build. All three iOS app bundles passed
their audits. The full build exposed an existing Ninja packaging issue: the
raw icon catalog was copied alongside its compiled assets. The catalog is now
an Xcode-only target source; Ninja uses the existing `actool` compilation path.

Clean iOS 27 Simulators for iPhone 17 Pro and iPad Pro 13-inch (M4) installed
and launched the app in landscape, remained running, and displayed the native
missing-game-data prompt. This verifies installation and initial launch only;
no game data, gameplay, touch acceptance, or performance was tested in that
smoke pass.

## Goal ledger

| Goal | State | Evidence or remaining gate |
| --- | --- | --- |
| [0 — Safe workspace](../GOAL-LOOP.md#goal-0--safe-reproducible-workspace) | Complete | `ipad` is isolated from the read-only upstream mirror; repository safety checks and ignored `ref/` handling are in place. |
| [1 — macOS keystone](../GOAL-LOOP.md#goal-1--macos-keystone) | Complete | Native arm64 build and deterministic headless simulation pass with repo-local paths. |
| [2 — iOS build contract](../GOAL-LOOP.md#goal-2--ios-build-contract-and-dependency-closure) | Complete | Device and Simulator dependency slices link; versions, hashes, flags, and licences are recorded in [`vendor/MANIFEST.md`](../vendor/MANIFEST.md). |
| [3 — Simulator app boot](../GOAL-LOOP.md#goal-3--simulator-app-boot) | Complete | The UIKit/SDL application launches, streams logs, and reaches the OpenRCT2 UI in an iPad Simulator. |
| [4 — Framebuffer presentation](../GOAL-LOOP.md#goal-4--correct-software-framebuffer-presentation) | Complete | The software framebuffer presents through SDL's iOS Metal renderer in landscape at Retina scale. |
| [5 — Sandbox and import](../GOAL-LOOP.md#goal-5--sandbox-paths-and-user-owned-data-import) | Complete | A clean physical-iPad install selected standard RCT2 data through Files, validated and copied it safely, retained it after forced relaunch, and loaded a scenario. RCT Classic and malformed-folder paths are also covered in Simulator. |
| [6 — Pointer, keyboard, mouse](../GOAL-LOOP.md#goal-6--pointer-keyboard-and-mouse-play) | In progress | Pointer movement, clicking, scrolling, trackpad zoom, attached-keyboard text entry, and existing controls work on device. The recorded coaster-and-scenery end-to-end proof remains. |
| [7 — Finger-first controls](../GOAL-LOOP.md#goal-7--finger-first-controls) | In progress | The current touch mapping is accepted on device: tap/placement, UI dragging, long-press secondary action, inverted half-speed pan, pinch, native text entry, paint/remove dragging, and construction rotation. The full finger-only coaster-and-scenery proof remains. |
| [8 — Release hygiene](../GOAL-LOOP.md#goal-8--release-documentation-and-package-hygiene) | Complete | Canonical install docs, honest feature boundaries, fork support paths, maintainer-owned bundle identity, CI safety checks, and embedded licence notices pass clean device and Simulator bundle audits. |
| [9 — Performance and stability](../GOAL-LOOP.md#goal-9--device-performance-and-stability) | Not started | Requires repeatable physical-device profiling and stress testing. |
| [10 — Content and installable MVP](../GOAL-LOOP.md#goal-10--plugincustom-content-and-installable-mvp) | Not started | Requires on-device plugin/custom-content proof, packaging audit, and the full MVP demo. |

## Current proof

- The Touch branch is synchronized through upstream OpenRCT2 `develop` commit
  `69872010ae6b` from July 31, 2026. The merge passed repository safety, the
  macOS build and 1,000-tick headless run, both iOS dependency slices, both iOS
  app bundles, and the full iPad Simulator lifecycle verification.
- `./scripts/build-ios.sh all` builds and audits both iOS device and Simulator
  application bundles.
- `./scripts/package-ios-ipa.sh` creates an audited unsigned ROM-free IPA under
  `build/packages/`. This is a local artifact and does not satisfy the signed
  binary-beta gates in [`RELEASE-CHECKLIST.md`](RELEASE-CHECKLIST.md).
- `./scripts/build-ios-device.sh unsigned` produces an Xcode device build
  without requiring repository-held signing identity data.
- Build 3 signs, audits, installs, and launches on the physical M2 iPad Pro
  under `com.chrissotraidis.openrct2touch`. Live logs confirm a read-only
  bundle, writable Documents/Library paths, and the first-run Files flow.
- On iPadOS 26.5.2, the tester selected the user-owned standard RCT2 folder
  through Files. The app acquired security-scoped access, validated
  `Data/g1.dat`, completed the copy into `Documents/rct2`, and indexed 2,518
  objects, 204 track designs, and 57 scenarios.
- A forced process termination and relaunch reused the same sandbox without
  showing the importer again. The tester then loaded a scenario and reported
  that the game loads cleanly and works without an observed issue.
- The physical run exposed a picker-presentation bug while the keyboard window
  was active. The importer now accepts only visible foreground normal-level app
  windows; the rebuilt app presented Files successfully with the Magic Keyboard
  attached.
- The Simulator Files picker successfully imports both the standard
  `Data/g1.dat` layout and RCT Classic's `Assets/g1.dat` layout. It preserves
  the source, persists the destination, repairs the stored path after app
  updates, and leaves no temporary or backup debris.
- A malformed folder is rejected with a useful error and no partial import.
- The local developer flow loads a real scenario. Proprietary game data remains
  outside tracked files and distributable bundles.
- The attached Magic Keyboard and trackpad retain their existing behavior.
  Finger controls support placement, long press, responsive inverted
  two-finger panning, and a more deliberate pinch threshold. The app-owned
  keyboard fallback was removed after device testing found that it obscured
  in-game text fields.
- The physical-device tester reports that the build appears stable and works
  with good controls. This is interaction acceptance for the current slice,
  not the Goal 9 performance or 30-minute stress proof.
- The July 16 device pass accepted native conditional text entry, one-finger
  placement and paint dragging, two-finger pan and pinch, 15-degree
  construction rotation, and two-finger tap or hold-drag footpath removal.
- The production `AppIcon` asset catalog compiles an opaque 1024px source into
  the required iPad icon renditions. Build 3 is installed and device-accepted
  under `com.chrissotraidis.openrct2touch`.
- The macOS headless loop completed 1,000 ticks with checksum
  `25232284e49cf2cb000000000000000000000000`.
- Goal 8 release hygiene passes: `./scripts/check-touch-release-safety.sh`,
  `./scripts/build-ios.sh all`, both strengthened bundle audits, and the macOS
  headless loop are green. GitHub Issues is enabled with an iPad-specific form.
- A clean iPad Pro 13-inch (M4) Simulator sandbox on iPadOS 26.5 launches
  without proprietary data and reaches the expected import prompt. Three
  external iPadOS 27 developer-beta crash reports identify the same UIKit
  `UIApplicationEvaluateRuntimeIssueForNoSceneLifecycleAdoption` trap before
  engine initialization. The iOS dependency now carries a focused backport of
  SDL's upstream UIScene lifecycle fix; physical iPadOS 27 confirmation remains
  pending. The install flow saves its launch console, and
  `scripts/collect-crash.sh` retrieves matching CoreDevice crash reports without
  copying game data.

## Current interaction checkpoint

- Native text entry is device-confirmed: iPadOS shows its software keyboard when
  the hardware keyboard is detached and correctly leaves it hidden while the
  attached keyboard is available.
- Active placement follows one-finger movement correctly. The confirmation
  double-tap window at 390 ms is accepted at the current checkpoint.
- Hold-then-drag now maps to the engine's existing mouse tool-drag behavior for
  footpath, land, water, and clear-scenery tools. Footpath painting is
  device-confirmed. Two-finger secondary tap removes one path segment and a
  two-finger hold-drag removes continuously; both are device-confirmed without
  changing mouse or trackpad right-click removal.
- A bounded two-finger rotation prototype is enabled only for active scenery,
  ride-construction, and track-design placement tools. Its 15-degree start
  threshold and delayed pan arbitration are accepted at this checkpoint.

The full mapping and decision history are maintained in
[`TOUCH-CONTROLS.md`](TOUCH-CONTROLS.md).

## Physical-device checkpoint

The July 16 signed-device loop used the existing local development signing
configuration without recording team or device identifiers in the repository:

```bash
OPENRCT2_DEVELOPMENT_TEAM=<team> \
OPENRCT2_DEVICE_UDID=<device> \
    ./scripts/build-ios-device.sh signed
OPENRCT2_DEVICE_UDID=<device> ./scripts/install-run-ios.sh
```

The signed build, install, launch, bundle audit, macOS build, repository safety
check, and 1,000-tick headless smoke all passed. On iPadOS 26.5.2, the physical
Files import, forced relaunch persistence check, and scenario load also passed.

## Remaining human gates

- Record the full Goal 6 coaster build and scenery placement using only the
  attached pointer/keyboard controls.
- Record the full Goal 7 coaster build and scenery placement using only fingers,
  including text entry. Revisit feel only if that longer flow exposes a gap.

## Remaining goal loop

Close the remaining proofs one goal at a time using the loop in
[`GOAL-LOOP.md`](../GOAL-LOOP.md#the-loop) and the engineering detail in the
master plan's [phase ladder](openrct2-ipados-BUILD-PLAN.md#8-the-phase-by-phase-engineering-plan):

1. **Finish Goal 6:** record a scenario played end-to-end with the already-
   working trackpad or mouse and keyboard, including left/right/middle click,
   scroll, text, shortcuts, coaster construction, and scenery placement.
2. **Finish Goal 7:** record the same MVP flow with the current finger controls,
   including pan, zoom, long press, placement, window interaction, and touch
   text entry.
3. **Goal 9:** sustain at least 30 fps on the agreed mid-size park and complete a
   30-minute stress run without an out-of-memory failure; fix the top three
   reproducible device crashes.
4. **Goal 10:** load one plugin or custom scenario, audit the distributable for
   proprietary assets, install through the chosen private channel, and pass the
   complete physical-iPad MVP demo.

Every goal closes only when its exit proof in [`GOAL-LOOP.md`](../GOAL-LOOP.md#goal-ladder-and-exit-proofs)
is green. Physical signing, touch feel, and any asset-packaging risk stay
explicit human gates.
