# OpenRCT2 Touch — Master Build Plan
### A native iPadOS port of OpenRCT2 (touch-first + hardware pointers)

**Project name:** OpenRCT2Touch (display: "OpenRCT2 Touch") — an unofficial, community iPad port of OpenRCT2. Repo/codename: `openrct2-touch`.
**Author:** Chris (with Claude)
**Date:** July 13, 2026
**Companion doc:** `openrct2-ipados-feasibility.md` (the research/verdict). This document is the *how*.
**Status:** Active implementation. Goals 0–5 and Goal 8 are complete; Goal 6 is the current formal proof. See [`DEVELOPMENT-STATUS.md`](DEVELOPMENT-STATUS.md) for the live checkpoint and remaining goals. This document remains the master engineering plan from empty folder to working demo.

---

## 0. How to read this document

This is a build bible, not a quick-start. It is deliberately exhaustive so an agent can execute long stretches without you, and so *you* always know what "done" looks like at each gate.

Structure:
- **§1–4** — what we're building, the strategy, what you need before starting, and the all-important `ref/` folder for the copyrighted game data.
- **§5–7** — repo layout, the upstream code you're grafting onto, and the build-system plan.
- **§8** — the phase-by-phase engineering plan. This is the spine. Each phase has: **Goal · Files · Tasks · Exit criteria · Who drives (agent vs. you)**.
- **§9–10** — the automation harness and the `AGENTS.md` you hand the agent.
- **§11–15** — testing, distribution/legal, risk register, definition of done, and an appendix with the exact file inventory + command reference.

**Two rules that never bend:**
1. **Never commit or bundle the RCT2 game data.** It lives in `ref/` locally, is git-ignored, and is only ever *staged* into a build for your own testing. Shipping it is copyright infringement. (§4, §12.)
2. **macOS-headless-first.** The agent proves everything it can on a fast, deterministic desktop build before touching the iPad. iOS is the *outer* loop. (§2.)

---

## 1. What we're building

A **native iPadOS port of OpenRCT2** — the open-source RollerCoaster Tycoon 2 engine — with touch-first and hardware-pointer controls. Not RCT Classic. Not streaming. Not a VM. A real ARM64 app that runs on the iPad's own silicon. Apple Pencil and multiplayer are outside the current release scope.

### 1.1 Definition of done (the MVP demo)
The project is "done enough to show" when, on a physical iPad:
1. The app launches natively (no VM/stream), shows the OpenRCT2 title screen.
2. The user has imported their **own** RCT2 data via the Files app, and a scenario loads.
3. You can **build a working coaster and place scenery** using fingers or an attached mouse/trackpad, at a playable frame rate (≥ 30 fps on a mid-park).
4. Pinch-zoom and two-finger pan work; windows are usable.
5. **Bonus / the "why not just buy Classic" shot:** one community **plugin** or **custom scenario** loads on-device.

Everything else, including broad polish and public-store distribution, is out of scope for v1.

The app also enables experimental iPhone compatibility through its universal
iOS bundle configuration. iPad remains the primary tested platform and the MVP
acceptance target; iPhone layouts, controls, and performance require separate
physical-device validation.

Issue #16 adds optional discovery of user-owned RCT1 expansion data copied to
`Documents/rct1`, host-native macOS asset builds, and a configurable iOS bundle
identifier. The RCT1 picker remains outside this maintenance slice; the
upstream v0.5.7 upgrade remains a separate compatibility task.

### 1.2 Explicit non-goals for v1
- Public App Store release. Distribution is currently source build or a selected private beta channel; see §12.
- Apple Pencil and multiplayer support.
- Bundling any game assets.
- The OpenGL hardware renderer on iOS (we use the software renderer — §8 Phase 4).
- SDL3 migration (upstream is mid-migration; we stay on SDL2).
- Rewriting the in-engine UI. We *adapt input*, we don't rebuild windows.

---

## 2. Strategy & guiding principles

These decisions shape every phase. Read them once; they explain the "why" behind the ordering.

**2.1 Two loops, different speeds.** There is a fast inner loop (macOS, headless, seconds) and a slow outer loop (iPad, device, minutes + human). The agent lives in the inner loop and only promotes known-good builds outward.

| Loop | Target | Speed | Who | Verifies |
|---|---|---|---|---|
| Inner | macOS from source, `--headless` | seconds | agent, unattended | compiles, boots, loads park, simulates, no crash |
| Middle | iOS Simulator | ~10–30s | agent, mostly unattended | boots, renders UI, asset import, input plumbing |
| Outer | Physical iPad | ~1–2 min + hands | you + agent | perf, memory, touch *feel* |

**2.2 macOS-from-source is the keystone.** Before any iOS work, get OpenRCT2 building from source on your Mac and running with all data paths pointed into the repo (§8 Phase 1). This gives the agent a self-contained, reproducible target it can hammer. Most logic bugs surface here, cheaply.

**2.3 Reuse the Apple code that already exists.** OpenRCT2 already has `Platform.macOS.mm`, `Platform.Posix.cpp`, and `UiContext.macOS.mm`. iOS is Darwin + UIKit. You are extending an Apple/POSIX path, not inventing one. SDL2 already ships an iOS backend. The Android port already proves the "SDL owns the window, runs C++ `main`" pattern on mobile.

**2.4 Gate the human-only work.** Three things an agent cannot do alone: (a) physically wire the iPad + approve signing, (b) build the iOS dependency binaries the first time (fiddly, weak signal), (c) judge whether controls *feel* good. Every phase marks whether it's agent-drivable or needs you.

**2.5 Small, reversible commits.** Every task is a commit with a green build. The agent must never leave the **working branch** red. Use a branch per phase, cut from the pinned base (§2.6).

**2.6 We work inside the fork (decided).** The project lives in the fork **`chrissotraidis/OpenRCT2Touch`** (forked from `OpenRCT2/OpenRCT2`, GPL-3.0). This is a real GitHub fork, *not* a fresh repo with a submodule — so the entire upstream tree (`src/`, `data/`, `cmake/`, `distribution/`, …) is already present at the repo root, and all iOS work is added *alongside* it (in `/ios`, `/scripts`, etc.). The `develop` branch is kept as a clean mirror of upstream for syncing; **all port work happens on a dedicated branch** (`ipad`). Pin that branch's base to a known-good commit/tag (a release tag like `v0.5.2`/`v0.5.3`, or a specific recent `develop` SHA) so a moving upstream can't break the build under you.

**2.7 Fork & git etiquette (the agent MUST follow this).** This is a public, GPL-licensed community project. Being a good fork is a hard requirement, not a nicety:
- **Never commit to `develop`.** Treat it as read-only upstream mirror. All work lands on the `ipad` working branch (and short-lived phase branches merged into it).
- **Never push to `OpenRCT2/OpenRCT2`. Never open a pull request against upstream.** This is a standalone fork; do not touch their issue tracker or PR queue.
- **Preserve provenance.** Keep every upstream `LICENSE`/`licence.txt`, per-file copyright header, and `contributors.md` intact. Never strip or rewrite them.
- **Keep the "forked from" link.** Don't detach the fork or rewrite `develop`'s history. No force-pushes to shared branches.
- **Mark your changes.** Prefix port commits so they're distinguishable from upstream, e.g. `[touch] Phase 3: add UiContext.iOS.mm`. State what changed (GPLv3 asks for this).
- **Attribute, don't imply endorsement.** The `README` credits OpenRCT2 up top and states clearly this is an unofficial, community port not affiliated with the OpenRCT2 team.
- **Sync deliberately.** When pulling upstream, update `develop` from upstream, then rebase/merge `ipad` onto a chosen pinned point — never blind-merge a moving target mid-phase.
- **Assets are sacred (see §4).** Never commit, never push, never package `/ref` or any RCT2 game data. If a change would put game assets into a tracked file or a distributable artifact — stop, it's a hard error.

---

## 3. Prerequisites

### 3.1 Hardware
- A **Mac with Apple Silicon** (M-series). Builds the engine, runs the Simulator, drives the device loop.
- A **physical iPad** — an M-series model is recommended for performance headroom. A USB-C cable for tethered install/log.
- Optional but recommended: a **Magic Keyboard/trackpad** for the iPad (unlocks the fastest "it just plays like the PC" milestone).

### 3.2 Accounts
- A free **Apple ID** is enough to start (build + run on *your* iPad; 7-day cert churn — see feasibility doc §6).
- Upgrade to the **$99 Apple Developer Program** only when you want TestFlight or to hand a build to anyone else. Not needed day one.

### 3.3 Software (install on the Mac)
- **Xcode** (latest) + Command Line Tools. Provides `xcodebuild`, `xcrun devicectl`, the iOS SDK, the Simulator, Metal toolchain.
- **Homebrew**, then: `cmake`, `ninja`, `pkg-config`, `git`, `ios-deploy`, `libimobiledevice` (`idevicesyslog`, `idevicecrashreport`), `create-dmg` (optional).
- Upstream OpenRCT2 macOS build deps (Homebrew): the project documents these; expect `sdl2`, `speexdsp`(legacy—may be gone), `freetype`, `fontconfig`, `libpng`, `libzip`, `openssl`, `icu4c`, `jansson`/`nlohmann-json`, `flac`, `libvorbis`, `libogg`, `zlib`, `zstd`, `nasm`. (The agent should read upstream's own `readme.txt` / macOS CI to get the *current* exact list — this is a moving target.)

> **Agent note:** verify the current dependency set from the repo's `distribution/readme.txt` and `.github/workflows/*.yml` (macOS job) rather than trusting any static list, including this one.

---

## 4. The `ref/` folder — copyrighted game data

This is the folder Chris asked about specifically. It is where the **original RollerCoaster Tycoon 2 data files** live so the engine (and the agent's tests) can reference them. OpenRCT2 is an engine reimplementation; it cannot run without these.

### 4.1 What goes in it
The contents of a legitimate **RCT2** install (Steam / GOG / CD) — critically the `Data/` folder containing `g1.dat`, plus `ObjData/`, `Tracks/`, `Landscapes/`, `Scenarios/`, `Saved Games/`, and the `Data/*.dat` audio. OpenRCT2 validates the path by checking for `Data/g1.dat` (confirmed in `RootCommands.cpp` → `set-rct2`). RCT Classic data is also accepted by upstream as an alternative source; RCT1 data is optional (enables extra content).

### 4.2 How to get it onto the Mac
Buy RCT2 on **GOG** or **Steam** (cheap), install on a PC/Mac, and copy the install directory into `ref/rct2/`. On GOG the layout already looks like `.../RollerCoaster Tycoon 2/Data/g1.dat`. That's the folder you point `--rct2-data-path` at.

### 4.3 Structure
```
ref/                      # git-ignored in its entirety
├── rct2/                 # a full RCT2 install; must contain Data/g1.dat
│   ├── Data/
│   ├── ObjData/
│   ├── Tracks/
│   └── ...
├── rct1/                 # OPTIONAL original RCT1 install (Data/csg1.dat)
└── README.md             # NOT ignored: explains what to put here + legal note
```
Only `ref/README.md` is committed (a placeholder telling a future you/collaborator what belongs here and that it must be their own copy). Everything else under `ref/` is git-ignored.

### 4.4 The rules
- **`ref/` is git-ignored.** Never committed, never pushed.
- **`ref/` is never packaged into a shippable artifact.** Not into the macOS app for distribution, not into the iOS `.ipa` you'd give anyone. (For your *own* on-device testing you may stage a copy into the sandbox — that's fine because it never leaves your devices.)
- The **real** iOS onboarding is a Files-app import (§8 Phase 5). `ref/` is a developer convenience that stands in for "the user imported their copy," so the agent can test without a human tapping through a document picker every run.

---

## 5. Repository layout

This is the fork **`chrissotraidis/OpenRCT2Touch`** — the whole upstream tree is already present at the repo root. Your additions sit *alongside* it, and the port's engine/UI source files go **into the upstream dirs where they belong** (so CMake compiles them beside `Platform.macOS.mm`). Everything the build/tests reference lives under the repo root. All committed on the **`ipad`** branch, never `develop`.

```
OpenRCT2Touch/                 # your fork · work on branch `ipad`
│
│  ═══ UPSTREAM (already here from the fork — keep intact, patch in place) ═══
├── src/                       # engine + UI (patch per-OS files in place)
│   ├── openrct2/              #   platform/Platform.iOS.mm  ← NEW (beside Platform.macOS.mm)
│   ├── openrct2-ui/           #   UiContext.iOS.mm          ← NEW (beside UiContext.macOS.mm); input/, drawing/
│   └── openrct2-android/      #   mobile blueprint (reference only)
├── data/  cmake/  distribution/  resources/  .github/   # upstream dirs
├── CMakeLists.txt             # upstream root CMake  ← extended for the iOS target
├── licence.txt  contributors.md  readme.txt             # KEEP INTACT (attribution)
│
│  ═══ YOUR ADDITIONS (new files, ipad branch) ═══
├── AGENTS.md                  # agent operating manual (§10)
├── PLAN.md                    # this document (or a link to it)
├── README-touch.md            # port README: credit + "unofficial" disclaimer + build steps
├── .gitignore                 # APPEND port ignores to the upstream file (§5.1)
│
├── ios/                       # net-new: app shell + toolchain (no upstream home)
│   ├── App/                  # the UIKit app shell (Xcode project lives here)
│   │   ├── OpenRCT2Touch.xcodeproj
│   │   ├── AppDelegate.*, RootViewController, Info.plist
│   │   ├── OpenRCT2Touch.entitlements
│   │   └── Resources/        # staged engine data (built), app icon, launch screen
│   ├── toolchain/
│   │   └── ios.toolchain.cmake  # CMake iOS toolchain file
│   └── cmake/                # iOS target CMake fragments
│   # note: Platform.iOS.mm / UiContext.iOS.mm live in src/ (see above), not here
│
├── vendor/                    # cross-compiled iOS dependency binaries (git-ignored)
│   ├── ios-arm64/            # device slice (SDL2, ICU, freetype, png, ...)
│   └── ios-sim-arm64/        # simulator slice
│
├── ref/                       # copyrighted RCT2 data (git-ignored) — see §4
│
├── assets/
│   └── engine/               # OpenRCT2's OWN built data (g2.dat, object/, shaders/, language/)
│                             #   produced by the build; safe to distribute (GPL)
│
├── runtime/                   # writable dirs for macOS test runs (git-ignored)
│   ├── user/                 # saves, config.ini, plugins, screenshots
│   └── cache/
│
├── testdata/
│   ├── parks/                # a few .sc6/.sv6/.park for smoke tests (see licensing note)
│   └── plugins/              # a sample .js plugin for the on-device proof
│
├── build/                     # all build output (git-ignored)
│   ├── macos/
│   ├── ios-sim/
│   └── ios-device/
│
└── scripts/                   # the automation harness (§9)
    ├── env.sh                # defines ROOT and all data paths
    ├── bootstrap.sh          # clone check, brew deps, branch + ref/ sanity checks
    ├── build-macos.sh
    ├── run-macos-headless.sh # the fast smoke test
    ├── build-ios-deps.sh     # the dependency pit (Phase 2)
    ├── build-ios.sh          # xcodebuild wrapper (sim + device)
    ├── stage-assets.sh       # engine data + ref/ → bundle/sandbox
    ├── install-run-ios.sh    # devicectl/ios-deploy install + launch + log
    └── collect-crash.sh      # idevicecrashreport puller
```

### 5.1 `.gitignore` — APPEND to the upstream file (don't overwrite it)
The fork already ships an upstream `.gitignore`; preserve it and append:
```
# ═══ OpenRCT2Touch additions ═══
ref/                    # copyrighted RCT2 data — NEVER commit
!ref/README.md          # keep only the placeholder explaining what goes here
vendor/                 # cross-compiled iOS deps (rebuildable)
build/                  # build output
runtime/                # macOS test saves/config/cache
assets/engine/          # generated engine data (rebuildable)
*.ipa
*.mobileprovision
*.p12
**/DerivedData/
.DS_Store
```

---

## 6. Upstream architecture you're grafting onto

The agent must understand these load-bearing files before editing. All paths are relative to the fork's repo root (e.g. `src/openrct2/`). (Verified against `develop`, July 2026.)

### 6.1 The two libraries + the app
- **`src/openrct2/`** — `libopenrct2`, the engine (platform-agnostic core + a thin per-OS `platform/` layer).
- **`src/openrct2-ui/`** — `libopenrct2ui`, the SDL2-based front end: window/context, input, the software+OpenGL drawing engines, and the in-engine window system.
- **`src/openrct2/`** also builds the CLI/`main` entry; the macOS app wraps it in an `.app`.

### 6.2 Filesystem resolution (already mapped — see feasibility doc)
- **`src/openrct2/PlatformEnvironment.cpp` / `.h`** — the single resolver. Seven roots (`DirBase`: rct1, rct2, openrct2, user, config, cache, documentation). Everything the engine reads/writes goes through here. Physical locations come from the `Platform::` layer + config + CLI overrides.

### 6.3 The per-OS platform layer (where iOS files are added)
Directory `src/openrct2/platform/`:
```
Platform.h             # the interface (GetInstallPath, GetFolderPath, GetAssetPath, ...)
Platform.Common.cpp    # shared
Platform.Posix.cpp     # POSIX shared (Linux+macOS+iOS can share much of this)
Platform.macOS.mm      # Cocoa/Foundation path resolution  ← closest template for iOS
Platform.Linux.cpp
Platform.Win32.cpp
Platform.Android.cpp   # mobile precedent (external storage, asset dir)
Platform.Emscripten.cpp
```
→ **You add `Platform.iOS.mm`** (or extend `Platform.macOS.mm` with `TARGET_OS_IOS` guards). It returns sandbox paths for `GetFolderPath(userData/userConfig/userCache)`, the app bundle for `GetInstallPath()`/`GetAssetPath()`.

### 6.4 The UI / SDL context (where the window + input live)
Directory `src/openrct2-ui/`:
```
UiContext.cpp          # the big cross-platform SDL window/context (36 KB)
UiContext.macOS.mm     # macOS-specific glue  ← template for iOS
UiContext.Android.cpp  # mobile glue (tap=click hacks live in this world)
UiContext.Win32.cpp / .Linux.cpp
input/                 # mouse/keyboard/shortcut handling  ← touch/GCMouse hook in here
drawing/               # drawing engines incl. opengl/ (the GL 3.3 renderer to bypass)
CursorRepository.*     # hardware cursors (mostly moot on touch)
TextComposition.*      # IME/text input  ← wire to iOS on-screen keyboard
```
→ **You add `UiContext.iOS.mm`** and extend `input/` for touch/GameController.

### 6.5 Command line / run modes (the harness's best friend)
- **`src/openrct2/command_line/RootCommands.cpp`** — defines the flags and subcommands. Confirmed useful ones:
  - `--rct2-data-path`, `--rct1-data-path`, `--openrct2-data-path`, `--user-data-path` (absolute path overrides for the roots)
  - `--headless` (sets `gOpenRCT2Headless` + `gOpenRCT2NoGraphics`)
  - subcommands: `screenshot`, `simulate`, `sprite`, `parkinfo`, `scan-objects`, `set-rct2`
- These are how the agent runs deterministic, GUI-less smoke tests on macOS (§9).

### 6.6 Scripting (plugins)
- **`src/openrct2/scripting/`** + vendored **`src/thirdparty/quickjs-ng/`** — the JS plugin engine. **quickjs-ng is a no-JIT bytecode interpreter** (swapped in from Duktape in v0.5.0, PR #23465). It compiles cleanly for iOS and needs no special entitlement. This is the debunked "JIT blocker."

### 6.7 The Android port (the blueprint to imitate)
- **`src/openrct2-android/`** — Gradle project. `GameActivity extends org.libsdl.app.SDLActivity`; native lib load order in `getLibraries()`; assets bundled in APK since v0.5.0; RCT2 data read from `/sdcard/rct2`. The iOS equivalent: a UIKit shell that starts SDL, engine data in the bundle, RCT2 data imported into the sandbox.

---

## 7. Build-system plan

### 7.1 The shape of the iOS build
OpenRCT2 uses **CMake**. Two viable routes; pick **A**:

- **Route A (recommended): CMake → Xcode generator + an iOS toolchain file.** Use a maintained `ios.toolchain.cmake` (e.g. the widely-used `leetal/ios-cmake`) to set `CMAKE_SYSTEM_NAME=iOS`, architectures, deployment target, and bitcode/sim flags. Generate an Xcode project for `libopenrct2` + `libopenrct2ui` as static libraries, then link them into the hand-written UIKit app (`ios/App`). This keeps the engine building via its own CMake while the app shell is a normal Xcode target.
- **Route B: pure `xcodebuild` of a hand-maintained project** that compiles the engine sources directly. More control, more drift from upstream. Avoid unless Route A fights you.

### 7.2 Dependencies for iOS (the hard part)
Every native lib the engine links must exist as an **iOS static lib / `.xcframework`**, built twice (device `arm64`, simulator `arm64`). Options, in order of preference:
1. **Prebuilt where possible.** SDL2 ships an official iOS `.xcframework`. ICU, freetype, libpng, zlib, etc. can be built with established scripts.
2. **`vcpkg` with an iOS triplet** (`arm64-ios`, `arm64-ios-simulator`) for the long tail. This is likely the fastest path to a full set and is script-friendly for the agent.
3. **Hand-rolled build scripts** as a fallback for anything vcpkg lacks.

Output lands in `vendor/ios-arm64/` and `vendor/ios-sim-arm64/`, which the CMake/Xcode target adds to its search paths. **This is Phase 2 and the single most likely place to stall — budget for it and drive it with the agent, not autonomously.**

### 7.3 Feature flags to set for iOS
- `DISABLE_OPENGL=ON` (use software renderer; skip GL 3.3 core path).
- `DISABLE_NETWORK` / `DISABLE_HTTP` — start **ON** (disabled) to shrink the dependency set for first boot; re-enable later for multiplayer/plugins-from-URL.
- `ENABLE_SCRIPTING=ON` (quickjs-ng; keep the plugin engine available for the later on-device proof).
- `DISABLE_TTF` — decide based on whether you ship freetype early; keep font rendering if feasible.
- Breakpad/crash handler — off initially; iOS has its own crash reports.

---

## 8. The phase-by-phase engineering plan

Each phase is a branch. Do not advance until **Exit criteria** are green. "Driver" = who does the work.

---

### Phase 0 — Repo & tooling bootstrap
**Goal:** the fork is cloned, the `ipad` branch exists, scaffolding is added, and upstream builds *unmodified* on macOS.
**Driver:** you + agent.
**Status:** fork already created at `chrissotraidis/OpenRCT2Touch` ✅.
**Files:** `scripts/env.sh`, `scripts/bootstrap.sh`, appended `.gitignore`, `ref/README.md`, `README-touch.md`, `AGENTS.md` (stub), plus the new `ios/ scripts/ vendor/ ref/ assets/ runtime/ testdata/ build/` dirs.
**Tasks:**
1. `git clone` your fork locally. `git remote -v` should show your fork as `origin`. Add upstream as a read-only remote for syncing only: `git remote add upstream https://github.com/OpenRCT2/OpenRCT2` — **never push to it, never PR to it.**
2. **Create and check out the working branch** from a pinned base: `git checkout -b ipad v0.5.3` (a release tag) or a specific `develop` SHA. Leave `develop` as the untouched upstream mirror. **All work happens on `ipad`.**
3. Add the new dirs (§5) and **append** the port `.gitignore` block (§5.1) to the upstream `.gitignore`. **Confirm `ref/` is ignored (`git status`) before any commit that could touch game data.**
4. `scripts/env.sh` exports `ROOT`, `RCT2_DATA=$ROOT/ref/rct2`, `OPENRCT2_DATA=$ROOT/assets/engine`, `USER_DATA=$ROOT/runtime/user`.
5. `scripts/bootstrap.sh`: `brew install` deps, verify Xcode + CLI tools, assert current branch is `ipad`, assert `ref/` is untracked/ignored, verify `ref/rct2/Data/g1.dat` exists (warn if not).
6. Write `README-touch.md`: credit OpenRCT2 up top, "unofficial community port, not affiliated with the OpenRCT2 team," link upstream; keep `licence.txt`/`contributors.md` intact.
7. First commit on `ipad`, prefixed: `[touch] Phase 0: scaffolding + harness`.
**Exit criteria:** on branch `ipad`; `develop` untouched; `bootstrap.sh` runs clean; `ref/` present and confirmed git-ignored; a first `[touch]`-prefixed commit exists. (Phase 1 proves the actual build.)

---

### Phase 1 — macOS-from-source baseline (the keystone)
**Goal:** build OpenRCT2 from source and run it fully repo-local, including headless. This is the agent's fast loop; it must exist before iOS.
**Driver:** agent (you unblock brew issues).
**Files:** `scripts/build-macos.sh`, `scripts/run-macos-headless.sh`, `assets/engine/` populated by the build.
**Tasks:**
1. Configure + build upstream via CMake for macOS (native arm64). Produce the `openrct2` binary and the engine data (`assets/engine`).
2. `run-macos-headless.sh`: launch the intrinsically headless `simulate` subcommand with its path overrides pointed into the repo, using the tracked `test/tests/testdata/parks/tile-element-tests.sv6` fixture. The fork extends `simulate` to accept these path options; upstream's unmodified subcommand declares no options.
3. Add a GUI run script too (windowed) for your own eyeballing.
4. Capture a **baseline screenshot** via the `screenshot` subcommand for later visual diffs.
**Exit criteria:** headless run loads a park, simulates N ticks, exits 0, no crash. A windowed run plays with mouse/keyboard on the Mac using only repo-local data (nothing in `~/Library`). **This proves the whole data-path + harness model before iOS risk.**

---

### Phase 2 — iOS dependency toolchain (the pit)
**Goal:** every engine dependency exists as an iOS static lib for device + simulator.
**Driver:** **you + agent, closely.** Weak automated signal; expect iteration.
**Files:** `ios/toolchain/ios.toolchain.cmake`, `scripts/build-ios-deps.sh`, `vendor/ios-*/`.
**Tasks:**
1. Stand up the toolchain file (leetal/ios-cmake or equivalent).
2. Get SDL2's iOS `.xcframework`.
3. Build/obtain ICU, freetype, libpng, zlib, zstd, libzip, and (if kept) FLAC/ogg/vorbis, OpenSSL for both slices. Prefer vcpkg iOS triplets; script the rest.
4. Record exact versions + build flags in `vendor/MANIFEST.md` for reproducibility.
**Exit criteria:** a trivial C++ test target links against all of them and builds for `arm64-ios` and `arm64-ios-simulator`. (Don't involve the engine yet — isolate the dependency variable.)

---

### Phase 3 — iOS app shell + SDL boots to title (Simulator)
**Goal:** the engine runs under a UIKit app and reaches the title screen in the Simulator.
**Driver:** agent (you approve first device/sim run).
**Files:** `ios/App/*` (Xcode project, `AppDelegate`, root VC, `Info.plist`, entitlements), `ios/platform/UiContext.iOS.mm` (stub), CMake iOS target linking `libopenrct2` + `libopenrct2ui` + `vendor/` libs.
**Tasks:**
1. Create the UIKit app target. Mirror the SDL2 iOS template: SDL provides the `UIApplicationMain`/`SDL_main` bridge (the iOS analog of Android's `SDLActivity`). Confirm whether to use SDL's `SDL_uikit` main shim or a custom `AppDelegate` that hands off to `SDL_main`.
2. Link the engine static libs + deps. Resolve the inevitable missing-symbol/framework errors (UIKit, Metal, AudioToolbox, AVFoundation, GameController, CoreHaptics).
3. Bundle a minimal set of engine data (`assets/engine`) into `App/Resources` so `GetInstallPath()`/`GetAssetPath()` can find it (Phase 5 makes this robust).
4. Get it to launch in the **Simulator** and reach the title/intro (which needs engine data but not RCT2 data).
**Exit criteria:** app launches in the iPad Simulator and shows OpenRCT2's title screen (or a clean "no RCT2 data" prompt). Logs stream via Xcode console. No RCT2 data required yet.

---

### Phase 4 — Rendering: software renderer → Metal surface
**Goal:** the in-engine UI draws correctly on iOS via the software renderer; the GL path is disabled.
**Driver:** agent.
**Files:** CMake flags (`DISABLE_OPENGL=ON`), possibly small edits in `openrct2-ui/drawing/` and `UiContext.iOS.mm`.
**Tasks:**
1. Force the software drawing engine on iOS. Ensure the engine's framebuffer is uploaded to an `SDL_Texture` backed by Metal and presented (SDL's iOS renderer uses Metal by default).
2. Handle Retina scale (points vs. pixels), safe-area insets, and orientation (lock to landscape for v1).
3. Verify colors/palette and no tearing.
**Exit criteria:** title screen + an in-engine window render pixel-correct in the Simulator at Retina scale, landscape, correct aspect. Frame presented through Metal.

---

### Phase 5 — Filesystem, sandbox & asset import
**Goal:** real path resolution on iOS + the user-facing RCT2 import flow, with `ref/` as the dev stand-in.
**Driver:** agent (you test the real Files import once).
**Files:** `ios/platform/Platform.iOS.mm`, `scripts/stage-assets.sh`, a small import UI in `ios/App`.
**Tasks:**
1. Implement `Platform.iOS.mm`: `GetInstallPath()`/`GetAssetPath()` → app bundle; `GetFolderPath(userData/userConfig/userCache)` → sandbox `Documents`/`Library`. Add the iOS branch to `GetOpenRCT2DirectoryName()` semantics as needed.
2. `stage-assets.sh`: copy `assets/engine` → `App/Resources`; for **dev/test builds only**, copy `ref/rct2` → the app's sandbox `Documents/rct2` and seed `runtime/user` → sandbox, then set `Config::Get().general.rct2Path` accordingly (or call the equivalent of `set-rct2`).
3. Implement the **real** import path: `UIDocumentPickerViewController` (folder import) + security-scoped bookmarks → copy/reference RCT2 data into the sandbox → write `rct2Path` to `config.ini`. Expose the app's `Documents` in the Files app (`UIFileSharingEnabled` / `LSSupportsOpeningDocumentsInPlace`).
4. First-run UX: detect missing/invalid RCT2 data (no `Data/g1.dat`) → show import screen.
**Exit criteria:** with `ref/`-staged data, a real scenario loads on device/sim end-to-end. Separately, the manual Files-app import flow works once, by hand, on a real iPad.

---

### Phase 6 — Input I: pointer, keyboard, mouse (playable fast)
**Goal:** fully playable with a Magic Keyboard/trackpad or mouse — the quickest "it plays like the PC" milestone.
**Driver:** agent (you smoke-test with hardware).
**Files:** `openrct2-ui/input/*` extensions, `UiContext.iOS.mm`.
**Tasks:**
1. Wire **`GCMouse`** (Game Controller framework): left/right/middle buttons, scroll, delta coords → the engine's existing mouse input. Right-click is the workhorse; get it mapped.
2. Wire **`GCKeyboard`** → OpenRCT2 keyboard shortcuts.
3. Optionally adopt `UIPointerInteraction` for a polite system cursor when not captured.
4. Route SDL's iOS mouse events if simpler than GameController for v1 — evaluate both.
**Exit criteria:** on a real iPad with keyboard/trackpad (or mouse), you can play a scenario end-to-end: build a coaster, place scenery, use shortcuts. This is a legitimate demo on its own.

---

### Phase 7 — Input II: touch controls
**Goal:** finger-only play is genuinely usable (not the Android tap=click hack).
**Driver:** agent builds; **you judge feel.**
**Files:** `openrct2-ui/input/*` (new touch gesture layer), `UiContext.iOS.mm`.
**Tasks (map per feasibility doc §4.2):**
1. One-finger drag pan on terrain; **two-finger drag = pan** even in build mode; **pinch = zoom**; double-tap zoom-to-point.
2. **Long-press = right-click / context menu.** Consider a persistent Build/Demolish mode toggle so the common secondary action becomes a primary tap.
3. Tap-to-place with a **confirm affordance** (don't rely on precise release).
4. Enlarge UI hit targets; drag-window-to-edge to dismiss; make in-engine windows finger-usable.
5. Multi-select via a dedicated tool with drag-rectangle.
**Exit criteria:** you can complete the MVP demo (build coaster, place scenery, navigate) **with fingers only**, and it feels acceptable. Iterate with the agent on the rough spots.

---

### Phase 8 — Release documentation and package hygiene
**Goal:** make every public claim, install step, support route, and distributable notice match the implementation.
**Driver:** agent, with a human review of release language.
**Files:** root README, `CONTRIBUTING.md`, issue templates, `NOTICE.md`, release checklist, bundle staging and audit scripts.
**Tasks:**
1. Keep one canonical physical-iPad installation guide with signing, data import, persistence, and troubleshooting steps.
2. Mark Apple Pencil and multiplayer unsupported; keep unproven plugins and performance as explicit gates.
3. Use a bundle identifier controlled by the fork maintainer and document the one-time sandbox migration.
4. Embed GPL, attribution, dependency manifest, and third-party licence texts in every iOS bundle.
5. Keep iPad support in this fork and preserve upstream contribution guidance for general OpenRCT2 work.
**Exit criteria:** the documentation audit is clean, every iOS bundle passes the strengthened package audit, and no proprietary data or signing identity enters the repository or bundle.

---

### Phase 9 — On-device performance, memory, stability
**Goal:** playable frame rate and no memory crashes on a real iPad, on a non-trivial park.
**Driver:** agent (automated perf capture) + you.
**Files:** profiling scripts, targeted engine tweaks.
**Tasks:**
1. Profile with Metal System Trace / Instruments; measure fps on a medium and large park.
2. Watch memory ceiling (the C&C port hit 3 GB+ and crashed on long sessions — RCT parks can be large). Fix leaks/peaks; consider capping park size or view distance on iOS.
3. Stress: long session, rapid zoom, big coaster. Fix top crashes from `idevicecrashreport`.
**Exit criteria:** ≥ 30 fps on a mid-size park; a 30-minute session doesn't OOM; top-3 device crashes fixed.

---

### Phase 10 — Plugins, custom content, packaging
**Goal:** prove the OpenRCT2-vs-Classic differentiators on-device, then package for TestFlight/sideload.
**Driver:** agent + you.
**Files:** `testdata/plugins/`, `testdata/parks/`, packaging scripts, entitlements/provisioning.
**Tasks:**
1. Load a sample **JS plugin** on-device (proves quickjs-ng runs interpreted on iOS — the debunked-JIT victory lap).
2. Load a **custom scenario / community park** on-device.
3. Keep networking disabled for this release and record that limitation.
4. Package only after selecting a registered-device or TestFlight channel; document and verify the exact archive/export steps for that channel.
**Exit criteria:** the full MVP demo (§1.1) runs on a physical iPad, including a plugin/custom-scenario. A build is installable on a second device via your chosen channel.

---

## 9. The automation harness

This is what lets the agent "chew through it." The principle: **every change ends in a command that returns a machine-readable pass/fail.**

### 9.1 Fast inner loop (macOS, unattended)
`scripts/run-macos-headless.sh`:
```bash
source scripts/env.sh
"$ROOT/build/macos/install/bin/openrct2" \
  simulate "$ROOT/test/tests/testdata/parks/tile-element-tests.sv6" 5000 \
  --rct2-data-path "$RCT2_DATA" \
  --openrct2-data-path "$OPENRCT2_DATA" \
  --user-data-path "$USER_DATA"
echo "exit=$?"
```
Wrap in a script that: builds → runs headless simulate on several parks → runs `screenshot` and diffs against a baseline → greps logs for `assert`/`FATAL` → returns non-zero on any failure. The agent loops: edit → this → read failure → patch.

### 9.2 Middle loop (Simulator)
`scripts/build-ios.sh sim` → `xcrun simctl boot` → `install` → `launch --console`. Good for UI/plumbing/asset-import checks without hardware. Mostly agent-drivable.

### 9.3 Outer loop (device)
`scripts/install-run-ios.sh`:
- Build: `xcodebuild -scheme OpenRCT2Touch -destination 'generic/platform=iOS' ...`
- Install + launch (iOS 17+): `xcrun devicectl device install app --device <UDID> <app>` then `... process launch --console --terminate-existing --device <UDID> <bundle-id>`
- iOS ≤16 fallback: `ios-deploy --debug --bundle <app>`
- Logs: `--console` (stdout) + `idevicesyslog` (system)
- Crashes: `scripts/collect-crash.sh` uses CoreDevice's `systemCrashLogs`
  domain to copy matching OpenRCT2Touch reports into `runtime/device-crashes/`;
  inspect the newest `.ips` before changing runtime code.

### 9.4 What the agent watches for
- Build failure (compiler/linker) → parse, fix, rebuild.
- Boot failure / assert / `FATAL` in logs → patch.
- Crash report present → symbolicate top frame → patch.
- Screenshot diff beyond threshold → flag for **human** (visual regressions need eyes).
- Perf below target → flag for human + capture trace.

### 9.5 The human gates (agent must stop and ask)
- First device provisioning / signing trust.
- Any "does this *feel* right" judgment (Phases 7–8).
- Dependency-build dead ends (Phase 2) after N failed attempts.
- Anything touching `ref/` staging into a *distributable* artifact (should never happen — hard stop).

---

## 10. `AGENTS.md` — what to hand the agent

Keep it short, imperative, and pointing here for depth. Contents:

1. **Mission:** port OpenRCT2 to native iPadOS; success = MVP demo (§1.1).
2. **Golden rules (non-negotiable):**
   - **Assets:** never commit, push, or package `ref/` contents or any RCT2 game data. `ref/` is git-ignored; it's the user's own copy for testing only. Putting assets into a tracked file or a distributable artifact = hard stop.
   - **Branch discipline:** all work on the **`ipad`** branch (or short-lived phase branches merged into it). **Never commit to `develop`** — it's the read-only upstream mirror. **Never push to `upstream` / `OpenRCT2/OpenRCT2`; never open a PR against it.**
   - **Provenance:** keep every upstream `licence.txt`, copyright header, and `contributors.md` intact. Don't rewrite `develop` history; no force-pushes to shared branches. Keep the "forked from" link.
   - **Commit hygiene:** small, reversible commits, each with a green build; prefix port commits `[touch] …` and state what changed. Never leave `ipad` red.
   - **macOS-headless-first:** prove changes in the inner loop before iOS.
   - **Attribution:** the README credits OpenRCT2 and states this is an unofficial, unaffiliated community port.
   - **Sync deliberately:** update `develop` from `upstream`, then rebase/merge `ipad` onto a chosen pinned point — never blind-merge a moving target mid-phase.
   - **Stop and ask at the human gates (§9.5).**
3. **How to build/test:** the exact `scripts/*` commands for each loop.
4. **Where things live:** the §6 file map (platform layer, UiContext, PlatformEnvironment, input/, scripting).
5. **Current phase + exit criteria:** update this field as you progress; the agent reads it to know its target.
6. **Definition of done per phase:** copy the Exit criteria from §8.
7. **Dependency manifest:** point to `vendor/MANIFEST.md`.

> Treat `AGENTS.md` as the agent's working memory; treat this `PLAN.md` as the reference manual.

---

## 11. Testing & QA strategy

- **Inner-loop regression:** headless `simulate` + `screenshot`-diff on a fixed park set, every change. Cheap, deterministic, catches logic breaks.
- **Simulator:** UI rendering, asset-import flow, input event plumbing.
- **Device-only (must):** frame rate, memory, trackpad/mouse, touch feel, thermals, orientation/safe-area, and real Files import.
- **Crash triage:** `idevicecrashreport` on every device session; keep a running top-crashes list in `AGENTS.md`.
- **Visual diffs need a human.** Automate the *detection*, not the *judgment*.
- **Verification agent (optional):** periodically spawn a second agent to audit the diff for regressions, re-run the full smoke suite, and sanity-check that no `ref/` content leaked into tracked files or the app bundle's distributable path.

---

## 12. Distribution & legal checklist

- **License:** OpenRCT2 is **GPLv3-or-later**. Your fork inherits it. Publish your iOS additions under GPLv3 too.
- **Public App Store:** not a current distribution target. GPLv3 compliance and Apple's current distribution terms require project-specific legal and maintainer review before that could change; do not present this engineering plan as legal advice.
- **Sideload (free provisioning):** fine for you; 7-day cert churn; 3 apps at once.
- **TestFlight:** needs the $99 program; internal testers = no review; external = Beta App Review + 90-day build expiry. Legit as a beta channel; don't use it to dodge review permanently.
- **EU alternative marketplaces:** possible post-DMA, but still require Apple Notarization.
- **Assets:** ship **none**. Users bring their own RCT2/RCT Classic data (Files import). This is both the legal position and the reason App Store distribution is moot.
- **Attribution:** keep upstream `licence.txt`, `contributors.md`; add a NOTICE crediting OpenRCT2.
- **Framing (reputational):** present as a **human-directed AI port / fork**, honestly. The RCT and open-source communities are sensitive to AI-generated code dumped upstream (the GZDoom episode). Keep it a fork; don't spam upstream with unrequested PRs.

---

## 13. Risk register (ranked) + mitigations

1. **iOS dependency cross-compile stalls (Phase 2).** *Mitigation:* isolate it before touching the engine; prefer vcpkg iOS triplets + SDL's official xcframework; record a MANIFEST; drive with the human, not autonomously.
2. **Software-renderer performance on large parks (Phase 9).** *Mitigation:* profile early on device; cap park size/view distance on iOS if needed; revisit ANGLE/Metal GL only if necessary.
3. **Touch *feel* (Phase 7).** *Mitigation:* human-in-loop; copy proven patterns (Civ VI, RCT Classic, OpenTTD Android); retain keyboard/trackpad play (Phase 6) as an alternative.
4. **Memory ceiling / OOM on device (Phase 9).** *Mitigation:* budget memory; test long sessions; fix peaks; consider smaller default parks.
5. **Asset-import UX friction (Phase 5).** *Mitigation:* `ref/` dev stand-in for testing; polish the Files import; clear first-run guidance.
6. **Upstream drift (SDL3, engine changes).** *Mitigation:* pin the `ipad` branch's base commit; sync/rebase deliberately; stay on SDL2 for v1.
7. **Signing/provisioning friction.** *Mitigation:* start free; buy the $99 program when distributing.
8. **Community/reputational blowback.** *Mitigation:* honest framing; fork not upstream-dump; credit maintainers.

*(Not on this list: JIT/plugins and SDL-on-iOS — both debunked. quickjs-ng is a no-JIT interpreter; SDL2/3 support iOS.)*

---

## 14. Definition of done + the demo script

**Done (v1):** the MVP in §1.1 runs on a physical iPad.

**The 30–60s demo (film this):**
1. Cold-launch on iPad — OpenRCT2 title, natively. (Establish: not a stream, not a VM.)
2. Open a park.
3. Build and run a coaster using the accepted finger placement and construction gestures.
4. Pinch-zoom the park; two-finger pan; tap-place scenery.
5. Load a **community plugin or custom scenario** — the instant answer to "why not just buy RCT Classic?"
6. Close line, honest: *"Directed by a human, ported by an AI agent from the open-source engine — the RollerCoaster Tycoon you grew up with, finally native on iPad, with everything OpenRCT2 adds that Classic never will."*

---

## 15. Appendix

### 15.1 File inventory — create vs. modify
**Create (new files on the `ipad` branch):**
- `src/openrct2/platform/Platform.iOS.mm` — path resolution for the sandbox/bundle (sits beside `Platform.macOS.mm`).
- `src/openrct2-ui/UiContext.iOS.mm` — SDL/UIKit window + input glue (sits beside `UiContext.macOS.mm`).
- Touch/GameController input source files under `src/openrct2-ui/input/`.
- `ios/App/*` — UIKit app shell, `Info.plist`, `OpenRCT2Touch.entitlements`, launch screen, icons.
- `ios/toolchain/ios.toolchain.cmake`, `ios/cmake/*` — iOS build config.
- All of `scripts/*`, `AGENTS.md`, `README-touch.md`, appended `.gitignore`, `ref/README.md`, `vendor/MANIFEST.md`.

**Modify (upstream files, patched in place on the `ipad` branch):**
- `src/openrct2/platform/*` + `Platform.h` — register the iOS platform (or `TARGET_OS_IOS` guards in `Platform.macOS.mm`/`Platform.Posix.cpp`).
- `src/openrct2/PlatformEnvironment.cpp` — iOS branch if `GetOpenRCT2DirectoryName()`/defaults need it.
- `src/openrct2-ui/UiContext.cpp` + `input/` + `drawing/` — iOS context, disable GL, touch/GC input.
- CMake files — add iOS target/flags (`DISABLE_OPENGL=ON`, etc.).

**Never touch for distribution:** `ref/` contents (data), any RCT2 asset.

### 15.2 Command reference (verified)
Data-path overrides (macOS/desktop): `--rct2-data-path`, `--rct1-data-path`, `--openrct2-data-path`, `--user-data-path` (all take absolute paths).
Run modes: `--headless`, subcommands `simulate <park> <ticks>`, `screenshot ...`, `scan-objects <path>`, `set-rct2 <path>` (validates `Data/g1.dat`).
iOS build/install: `xcodebuild`, `xcrun devicectl device install/process launch --console`, `ios-deploy --debug`, `idevicesyslog`, `idevicecrashreport`.

### 15.3 Key upstream references
- Repo: https://github.com/OpenRCT2/OpenRCT2
- Filesystem resolver: `src/openrct2/PlatformEnvironment.cpp` / `.h`
- Platform layer: `src/openrct2/platform/` (`Platform.macOS.mm`, `Platform.Posix.cpp`, `Platform.Android.cpp`, `Platform.h`)
- UI/SDL: `src/openrct2-ui/` (`UiContext.cpp`, `UiContext.macOS.mm`, `input/`, `drawing/`)
- CLI/flags: `src/openrct2/command_line/RootCommands.cpp`
- Scripting: `src/openrct2/scripting/` + `src/thirdparty/quickjs-ng/`
- Android blueprint: `src/openrct2-android/` (`GameActivity`, `getLibraries()`)
- Android install docs: https://docs.openrct2.io/en/latest/installing/installing-on-android.html
- Getting RCT2 data: https://docs.openrct2.io/en/latest/installing/getting-rct2.html
- SDL iOS: https://wiki.libsdl.org/SDL2/README-ios
- iOS CMake toolchain: https://github.com/leetal/ios-cmake
- Duktape→quickjs-ng: https://github.com/OpenRCT2/OpenRCT2/pull/23465

*(Full research citations: see `openrct2-ipados-feasibility.md`.)*

---

### Closing note
The engine is portable, quickjs-ng needs no JIT, SDL supports iOS, and the data model is already override-driven. The project's remaining difficulty is concentrated in the **complete on-device input proofs (Phases 6–7)**, **performance/stability work (Phase 9)**, and **custom-content/installable-MVP proof (Phase 10)**. Keep `ref/` sacred, prove public claims, and let the agent run the inner loop while the human owns the physical-device gates.
