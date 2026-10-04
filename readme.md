<p align="center">
  <img src="ios/App/Assets.xcassets/AppIcon.appiconset/AppIcon.png" alt="OpenRCT2 Touch app icon" width="180">
</p>

<h1 align="center">OpenRCT2 Touch</h1>

<p align="center">
  <strong>OpenRCT2, running natively on iPad with touch-first controls.</strong><br>
  A community iPadOS port built for user-owned RCT2 data, hardware pointers,
  fingers, keyboards, mice, and trackpads.
</p>

<p align="center">
  <a href="licence.txt"><img alt="GPL-3.0-or-later" src="https://img.shields.io/badge/license-GPL--3.0--or--later-4b7b4b?style=flat-square"></a>
  <img alt="iPadOS 15 or newer" src="https://img.shields.io/badge/iPadOS-15%2B-1f6f78?style=flat-square">
  <img alt="Native arm64" src="https://img.shields.io/badge/runtime-native%20arm64-d0953d?style=flat-square">
  <img alt="Physical device tested" src="https://img.shields.io/badge/device-M2%20iPad%20tested-4b7b4b?style=flat-square">
  <a href="https://discord.gg/xwHfUD2bxW"><img alt="Join the Discord" src="https://img.shields.io/badge/Discord-Join%20the%20community-5865F2?style=flat-square&amp;logo=discord&amp;logoColor=white"></a>
</p>

<p align="center">
  <a href="#what-works-today">Status</a> ·
  <a href="#install-on-an-ipad">Install</a> ·
  <a href="#touch-controls">Controls</a> ·
  <a href="#bring-your-own-game-data">Game data</a> ·
  <a href="#faq">FAQ</a> ·
  <a href="docs/DEVELOPMENT-STATUS.md">Engineering log</a>
</p>

> [!IMPORTANT]
> **OpenRCT2 Touch does not include RollerCoaster Tycoon or RollerCoaster
> Tycoon 2.** You provide data from your own legally owned copy. Proprietary
> game content is never tracked in this repository or placed in a distributable
> application bundle.

## What is OpenRCT2 Touch?

OpenRCT2 Touch is an unofficial iPadOS fork of the open-source
[OpenRCT2](https://github.com/OpenRCT2/OpenRCT2) engine. It runs as a real
ARM64 application on iPad—there is no Windows executable, x86 emulation,
virtual machine, or game streaming involved. The engine's software framebuffer
is presented through SDL's Metal renderer, while iPad-specific input translates
finger gestures into OpenRCT2's established construction and pointer actions.

The goal is a playable iPad experience that preserves OpenRCT2's scenarios,
parks, construction tools, and modern quality-of-life improvements. This
repository is currently a **source-only developer preview**. Signed development
builds run on a physical M2 iPad Pro, but there is no public IPA, TestFlight, or
App Store release.

The maintained and GitHub default branch for this fork is **`ipad`**. The
`develop` branch is retained only as an upstream mirror, and this project does
not use a `main` branch for Touch releases.

The August 1, 2026 checkpoint is synchronized through upstream OpenRCT2
`develop` commit `69872010ae6b`. Future updates should merge a tested upstream
commit into a short-lived `touch/*` branch, run the full macOS and iOS loops,
and merge the result into `ipad`; do not develop Touch changes on the mirror.

## What works today

| Area | State | Current evidence |
| --- | :---: | --- |
| Native iPad runtime | ✅ | Signed ARM64 iPadOS app, SDL Metal presentation, landscape Retina output |
| User data and persistence | ✅ | User-owned RCT2 data in the app sandbox survives build replacement and relaunch; scenarios load on device |
| Files folder import | ✅ | Standard RCT2 data imports through Files on a physical iPad, persists after relaunch, and loads scenarios; RCT Classic is also validated in Simulator |
| Keyboard and trackpad | ✅ | Pointer controls, shortcuts, text entry, scrolling, and tuned viewport zoom work on device |
| Finger controls | ✅ | Placement, path painting/removal, pan, pinch zoom, construction rotation, and native text entry accepted on device |
| Apple Pencil | — | Not supported; the current gesture model requires fingers or a hardware pointer |
| Multiplayer | — | Networking is disabled in the current iPadOS build |
| Performance and stability | 🧪 | Current play is stable; formal 30 fps and 30-minute stress proofs remain |
| Plugins/custom scenarios | 🧭 | On-device content proof and final packaging audit remain |

The verified checkpoint and remaining acceptance gates are kept in
[`docs/DEVELOPMENT-STATUS.md`](docs/DEVELOPMENT-STATUS.md) and
[`GOAL-LOOP.md`](GOAL-LOOP.md).

Build 3 is signed and installed on an iPad Pro (12.9-inch, 6th generation)
running iPadOS 26.5.2 under `com.chrissotraidis.openrct2touch`. A clean install
imports user-owned standard RCT2 data through Files, retains it after a forced
relaunch, and loads a scenario successfully.

## Touch controls

OpenRCT2 Touch keeps attached mouse, trackpad, and keyboard behavior intact.
Finger controls add the smallest gesture set needed to make construction
practical on glass.

| Gesture | Action |
| --- | --- |
| One-finger tap | Primary click and ordinary UI interaction |
| One-finger move during placement | Position the construction preview |
| Double tap during placement | Confirm placement |
| One-finger hold, then drag | Paint paths, terrain, water, or clear scenery |
| Two-finger drag | Pan the park at a controlled speed |
| Two-finger pinch | Zoom the park |
| Two-finger twist during placement | Rotate supported rides, scenery, and track designs |
| Two-finger tap on a path | Remove one path segment |
| Two-finger hold on a path, then drag | Remove path segments continuously |
| Long press outside paint tools | Secondary-click action |

The thresholds, gesture arbitration, rationale, and dated decision log live in
[`docs/TOUCH-CONTROLS.md`](docs/TOUCH-CONTROLS.md).

Apple Pencil is not currently supported. It cannot reproduce the multi-finger
pan, pinch, twist, or secondary-action gestures used by this build. Use fingers
or an attached mouse/trackpad instead.

## Bring your own game data

Like upstream OpenRCT2, this project is an engine and requires original data
from **RollerCoaster Tycoon 2** or **RollerCoaster Tycoon Classic** that you own.
The standard RCT2 `Data/g1.dat` layout and RCT Classic `Assets/g1.dat` layout
are supported by the import flow. Copy the complete, unmodified installation
folder to iCloud Drive, **On My iPad**, or an attached drive before first
launch. Select the folder containing `Data`, not `Data` itself.

Standard RCT2:

```text
RollerCoaster Tycoon 2/
├── Data/
│   └── g1.dat
├── ObjData/
├── Scenarios/
└── Tracks/
```

RCT Classic:

```text
RollerCoaster Tycoon Classic/
└── Assets/
    └── g1.dat
```

If the folder exists only on your Mac, install OpenRCT2 Touch first, connect the
iPad by USB, select it in Finder, open the **Files** tab, and drag the complete
installation folder into **OpenRCT2 Touch**. In the app's picker, choose
**On My iPad → OpenRCT2 Touch → your installation folder**. The app copies the
validated data into its private `Documents/rct2` directory; after a successful
launch, the staging folder you dragged into Files can be removed to reclaim
space.

```text
Your legally owned RCT2 or RCT Classic folder
                       │
                       ▼  choose through iPadOS Files
          private application Documents storage
                       │
                       ▼
          native OpenRCT2 engine on your iPad
```

Repository safety checks reject game data in tracked files and audit every iOS
bundle before packaging or installation. Only the explanatory
[`ref/README.md`](ref/README.md) may be tracked beneath `ref/`.

## Install on an iPad

There is no downloadable public build yet; [PadMint](https://github.com/chrissotraidis/padmint) lists OpenRCT2 Touch under
**Not available yet** and links back here. The supported installation method
for this developer preview is to build from source on a Mac and sign the app
for an iPad registered to your Apple development team.

### Requirements

- An Apple Silicon Mac.
- Full Xcode with an iOS SDK and command-line tools installed.
- CMake 3.24 or newer, Ninja, pkg-config, and Git.
- An iPad running iPadOS 15 or newer, connected by USB, unlocked, trusted, and
  with Developer Mode enabled.
- An Apple ID selected as a development team in Xcode. Free Personal Team
  builds expire after seven days and must be rebuilt and reinstalled.
- User-owned RCT2 or RCT Classic data placed somewhere accessible in Files.

### 1. Clone and check the toolchain

```sh
git clone https://github.com/chrissotraidis/OpenRCT2Touch.git
cd OpenRCT2Touch
git switch ipad

./scripts/bootstrap.sh --install
```

`bootstrap.sh` does not require game data for an iOS-only build. Continue
directly to step 2 if you only want to install the iPad app.

The following macOS/headless test is optional and is not part of the iPad
build. Run it only if you have a local RCT2 installation, then point
`RCT2_DATA` at the folder that contains `Data/g1.dat`:

```sh
RCT2_DATA="/absolute/path/to/RollerCoaster Tycoon 2" \
    ./scripts/bootstrap.sh --require-data
./scripts/build-macos.sh
RCT2_DATA="/absolute/path/to/RollerCoaster Tycoon 2" \
    ./scripts/run-macos-headless.sh
```

### 2. Build the pinned iOS dependencies

The first dependency build downloads and compiles the pinned open-source
dependency set and can take a while. Generated files remain ignored under
`vendor/` and `build/`.

```sh
./scripts/build-ios-deps.sh device
```

For an optional Simulator build, use:

```sh
./scripts/build-ios-deps.sh sim
./scripts/build-ios.sh sim
./scripts/run-ios-sim.sh verify
```

### Optional: create a local unsigned IPA

After building the device app, this command creates an audited, ROM-free IPA
under `build/packages/`:

```sh
./scripts/build-ios.sh device
./scripts/package-ios-ipa.sh
```

The resulting IPA contains no RCT/RCT2 game data, code signature, or
provisioning profile. It is a local build artifact for advanced signing and
packaging workflows, not a generally installable public release. Ordinary
users should continue with the signed source-build instructions below.

### 3. Find the connected device and select a team

List the iPad identifier:

```sh
xcrun devicectl list devices
```

List local code-signing identities with:

```sh
security find-identity -v -p codesigning
```

For an Apple Development identity, the value in parentheses at the end of the
identity name is the team identifier. You can also confirm the account and team
under **Xcode → Settings → Accounts**. Team and device identifiers are passed
only as environment variables; the scripts never save them in the repository.

### 4. Sign, install, and launch

```sh
OPENRCT2_DEVELOPMENT_TEAM=<team-identifier> \
OPENRCT2_DEVICE_UDID=<device-identifier> \
    ./scripts/build-ios-device.sh signed

OPENRCT2_DEVICE_UDID=<device-identifier> \
    ./scripts/install-run-ios.sh
```

The device build automatically creates the required redistributable OpenRCT2
engine assets, including `g2.dat`. These are mandatory app components, but they
are separate from user-owned RCT2 game data. Proprietary RCT2 data is not
needed until the first-launch Files import.

The install script reruns the repository and bundle audits before installing.
The bundle contains the OpenRCT2 engine, required open-source notices, and no
proprietary RCT2 data. It also saves the device launch console to
`runtime/device-logs/OpenRCT2Touch-launch.log`.

### 5. Import your game data

1. Launch OpenRCT2 Touch and tap **OK** on the missing-data explanation.
2. When the Files picker appears, select the installation folder described in
   [Bring your own game data](#bring-your-own-game-data).
3. Leave the app open while it validates and copies the folder into its private
   Documents container.
4. Load a scenario, quit the app, reopen it, and confirm the scenario remains
   available.

If validation fails, check that you selected the installation root and that
`Data/g1.dat` or `Assets/g1.dat` exists. Rebuilding with the same bundle
identifier preserves the sandbox. The July 2026 change from the old
`org.openrct2.touch` identifier to `com.chrissotraidis.openrct2touch` creates a
new sandbox, so existing development testers must import once after updating.

### Troubleshooting

- **No device appears:** unlock the iPad, accept the trust prompt, reconnect
  USB, and rerun `xcrun devicectl list devices`.
- **Developer Mode is required:** enable it under **Settings → Privacy &
  Security → Developer Mode**, restart the iPad, and confirm the prompt.
- **Signing fails:** open Xcode, sign into the intended Apple ID, and verify the
  team can register the connected device.
- **The app stopped launching after several days:** rebuild and reinstall; free
  Personal Team provisioning is temporary.
- **The software keyboard stays hidden:** disconnect or disable the attached
  hardware keyboard. iPadOS intentionally suppresses the software keyboard
  while a hardware keyboard is active.
- **The import picker does not appear:** make sure OpenRCT2 Touch is active,
  dismiss any software keyboard, and retry. If it still fails, capture the
  device log and file an iPad-port issue.
- **The app closes immediately:** rerun `./scripts/install-run-ios.sh` to save
  the launch console, reproduce the crash, wait a few seconds, and collect the
  iPad crash report with:

  ```sh
  OPENRCT2_DEVICE_UDID=<device-identifier> ./scripts/collect-crash.sh
  ```

  Attach the console log and the newest file from `runtime/device-crashes/` to
  an iPad-port issue. A crash on a beta iPadOS release cannot be diagnosed from
  the icon disappearing alone.
- **Two OpenRCT2 Touch icons appear after updating:** builds using the former
  `org.openrct2.touch` identifier can coexist with the current
  `com.chrissotraidis.openrct2touch` app. Do not delete the older app until you
  have accounted for any saves in its separate sandbox.

## Known limitations

- Apple Pencil and multiplayer are unsupported.
- Plugin and custom-scenario loading have not yet been proven on device.
- The formal 30 fps benchmark and 30-minute stress test remain outstanding.
- This project currently provides source and development signing workflows,
  not a generally installable IPA or TestFlight build.

See [`docs/RELEASE-CHECKLIST.md`](docs/RELEASE-CHECKLIST.md) for the exact source
preview and binary-release gates.

## FAQ

### Why is there no public IPA?

OpenRCT2 Touch can already produce an audited, unsigned `.ipa` locally. We have
not published it as a general download because creating the file and releasing a
legally compliant, installable binary are different things:

- **The app must not contain RollerCoaster Tycoon data.** OpenRCT2 needs graphics,
  sounds, models, and other files from RCT2 or RCT Classic. Those files are
  proprietary, so every user must import data from a copy they legally own.
- **The OpenRCT2 code is GPLv3-or-later.** Any binary release must preserve the
  licence and attribution, keep the Touch modifications under the GPL, and give
  recipients access to the exact corresponding source without adding
  incompatible restrictions.
- **iPad apps require an Apple signing and distribution arrangement.** An
  unsigned IPA from GitHub is not directly installable on an ordinary iPad.
  Registered-device distribution, TestFlight, the App Store, and eligible
  alternative distribution each have different provisioning and contract terms.
  The interaction between those terms and GPLv3 needs project-specific review
  before we choose a public binary channel.
- **The binary-release proof is not finished.** The complete pointer and
  finger-only playthroughs, performance and stress tests, plugin or custom-content
  proof, package audit, and second-device installation are still release gates.

For now, publishing source and letting users build and sign their own copy is the
clearest compliant path. This is not a claim that GPL software cannot run on
iPadOS, or that OpenRCT2 Touch can never have a binary release. A binary beta can
follow once the remaining checks pass and a suitable distribution channel has
been selected and reviewed. See the
[`release checklist`](docs/RELEASE-CHECKLIST.md) for the exact gates. This is a
project policy summary, not legal advice.

## Community and support

[Join the Discord](https://discord.gg/xwHfUD2bxW) for help and news. It is one
community for OpenRCT2 Touch and its sibling projects, such as KartPad, BlueWake
and RAtouch: ask about setup and installing, share how it runs on your iPad, and
hear about new releases first. The upstream OpenRCT2 chat links further down are
OpenRCT2's own.

Found a bug? [Open an issue](https://github.com/chrissotraidis/OpenRCT2Touch/issues)
with your iPad model, its iPadOS version, and the steps that led to it.

## Project boundary and credits

OpenRCT2 Touch is an independent, human-directed community port. It is not
affiliated with or endorsed by the OpenRCT2 team, Chris Sawyer, Atari, or the
owners of the RollerCoaster Tycoon trademarks. Report iPad-port issues in this
fork's [issue tracker](https://github.com/chrissotraidis/OpenRCT2Touch/issues),
not to upstream OpenRCT2.

The port exists because of the years of work by the
[OpenRCT2 developers and contributors](contributors.md). Their copyright
notices, project history, contribution material, and GPLv3-or-later licence are
preserved. New port code is distributed under the same licence; see
[`licence.txt`](licence.txt), [`NOTICE.md`](NOTICE.md), and
[`CONTRIBUTING.md`](CONTRIBUTING.md).

---

## Upstream OpenRCT2 project information

The original OpenRCT2 README is preserved below. Its downloads, community
links, supported desktop platforms, and contribution instructions describe the
upstream project, not a downloadable OpenRCT2 Touch iPad release.

<p align="center">
  <a href="https://openrct2.io">
    <img src="https://raw.githubusercontent.com/OpenRCT2/OpenRCT2/develop/resources/logo/icon_x128.png" style="width: 128px;" alt="OpenRCT2 logo"/>
  </a>
</p>

<h1 align="center">OpenRCT2</h1>

<h3 align="center">An open-source re-implementation of RollerCoaster Tycoon 2, a construction and management simulation video game that simulates amusement park management.</h3>

---

![Still from the v0.5.0 title sequence](https://github.com/user-attachments/assets/fa893cc8-1484-4751-94be-4ead00a6c8f9)


---

### Download
| Latest release                                                                                                       | Latest development build |
|----------------------------------------------------------------------------------------------------------------------|--------------------------|
| [![OpenRCT2.io](https://img.shields.io/github/v/release/OpenRCT2/OpenRCT2.svg?color=green)](https://openrct2.io/download/release/latest) | [![OpenRCT2.io](https://img.shields.io/github/last-commit/OpenRCT2/OpenRCT2/develop?color=green)](https://openrct2.io/download/develop/latest) |

---

### Chat
Chat takes place on Discord. You will need to create a Discord account if you don't yet have one.

If you want to help *make* the game, join the developer channel.

If you need help, want to talk to the developers, or just want to stay up to date then join the non-developer channel for your language.

If you want to help translate the game to your language, please stop by the Localisation channel.

| Language | Non Developer | Developer | Localisation | Asset Replacement |
| -------- | ------------- | --------- | ------------ | ----------------- |
| English | [![Discord](https://img.shields.io/badge/discord-%23openrct2--talk-blue.svg)](https://discord.gg/ZXZd8D8) </br> [![Discord](https://img.shields.io/badge/discord-%23help-blue.svg)](https://discord.gg/vJABqGGTEt) | [![Discord](https://img.shields.io/badge/discord-%23development-yellowgreen.svg)](https://discord.gg/fsEwSWs) | [![Discord](https://img.shields.io/badge/discord-%23localisation-green.svg)](https://discord.gg/sxnrvX9) | [![Discord](https://img.shields.io/badge/discord-%23open--graphics-b00b69.svg)](https://discord.gg/aM2Pchscnp) </br> [![Discord](https://img.shields.io/badge/discord-%23open--sound--and--music-b00b69.svg)](https://discord.gg/tuz3QBBWJf)
| Nederlands | [![Discord](https://img.shields.io/badge/discord-%23nederlands-orange.svg)](https://discord.gg/cQYSXzW) | | |

---

# Contents
- 1 - [Introduction](#1-introduction)
- 2 - [Downloading the game (pre-built)](#2-downloading-the-game-pre-built)
- 3 - [Building the game](#3-building-the-game)
- 4 - [Contributing](#4-contributing)
  - 4.1 - [Bug fixes](#41-bug-fixes)
  - 4.2 - [New features](#42-new-features)
  - 4.3 - [Translation](#43-translation)
  - 4.4 - [Graphics](#44-graphics)
  - 4.5 - [Audio](#45-audio)
  - 4.6 - [Scenarios](#46-scenarios)
- 5 - [Policies](#5-policies)
  - 5.1 - [Code of conduct](#51-code-of-conduct)
  - 5.2 - [Code signing policy](#52-code-signing-policy)
  - 5.3 - [Privacy policy](#53-privacy-policy)
- 6 - [Licence](#6-licence)
- 7 - [More information](#7-more-information)
- 8 - [Sponsors](#8-sponsors)

---

# 1. Introduction

**OpenRCT2** is an open-source re-implementation of RollerCoaster Tycoon 2 (RCT2). The gameplay revolves around building and maintaining an amusement park containing attractions, shops and facilities. The player must try to make a profit and maintain a good park reputation whilst keeping the guests happy. OpenRCT2 allows for both scenario and sandbox play. Scenarios require the player to complete a certain objective in a set time limit whilst sandbox allows the player to build a more flexible park with optionally no restrictions or finance.

RollerCoaster Tycoon 2 was originally written by Chris Sawyer in x86 assembly and is the sequel to RollerCoaster Tycoon. The engine was based on Transport Tycoon, an older game which also has an equivalent open-source project, [OpenTTD](https://openttd.org). OpenRCT2 attempts to provide everything from RCT2 as well as many improvements and additional features, some of these include support for modern platforms, an improved interface, improved guest and staff AI, more editing tools, increased limits, and cooperative multiplayer. It also re-introduces mechanics from RollerCoaster Tycoon that were not present in RollerCoaster Tycoon 2. Some of those include; mountain tool in-game, the *"have fun"* objective, launched coasters (not passing-through the station) and several buttons on the toolbar.

---

# 2. Downloading the game (pre-built)

OpenRCT2 requires original files of RollerCoaster Tycoon 2 to play. It can be bought at either [Steam](https://store.steampowered.com/app/285330/RollerCoaster_Tycoon_2_Triple_Thrill_Pack/) or [GOG.com](https://www.gog.com/game/rollercoaster_tycoon_2). If you have the original RollerCoaster Tycoon and its expansion packs, you can [point OpenRCT2 to these](https://github.com/OpenRCT2/OpenRCT2/wiki/Loading-RCT1-scenarios-and-data) in order to play the original scenarios.

[Our website](https://openrct2.io/download) offers portable builds and installers with the latest versions of the `master` and `develop` branches. There is also a [launcher](https://openrct2.io/download/launcher) available for Windows, macOS and Linux that will automatically update your build of the game so that you always have the latest version.

Alternatively to using the launcher, for most Linux distributions, we recommend the [latest Flatpak release](https://flathub.org/apps/details/io.openrct2.OpenRCT2). When downloading from Flathub, you will always receive the latest updates regardless of which Linux distribution you use.

Some Linux distributions offer native packages:
* Arch Linux: [openrct2](https://archlinux.org/packages/extra/x86_64/openrct2/) latest release (`extra` repository) and, alternatively, [openrct2-git](https://aur.archlinux.org/packages/openrct2-git) (AUR)
* Gentoo (main portage tree): [games-simulation/openrct2](https://packages.gentoo.org/packages/games-simulation/openrct2)
* NixOS: [openrct2](https://github.com/NixOS/nixpkgs/blob/master/pkgs/by-name/op/openrct2/package.nix)
* openSUSE OBS: [games/openrct2](https://software.opensuse.org/download.html?project=games&package=openrct2)
* Ubuntu PPA (nightly builds): [`develop` branch](https://launchpad.net/~openrct2/+archive/ubuntu/nightly)

Some \*BSD operating systems offer native packages:
* FreeBSD: [games/openrct2](https://www.freshports.org/games/openrct2)

---

# 3. Building the game
- [Building OpenRCT2 on Linux](https://github.com/OpenRCT2/OpenRCT2/wiki/Building-OpenRCT2-on-Linux)
- [Building OpenRCT2 on macOS using CMake](https://github.com/OpenRCT2/OpenRCT2/wiki/Building-OpenRCT2-on-macOS-using-CMake)
- [Building OpenRCT2 on Windows](https://github.com/OpenRCT2/OpenRCT2/wiki/Building-OpenRCT2-on-Windows)
- [Building OpenRCT2 on Windows Subsystem for Linux](https://github.com/OpenRCT2/OpenRCT2/wiki/Building-OpenRCT2-on-Windows-Subsystem-for-Linux)
- [Building OpenRCT2 on MSYS2 MinGW](https://github.com/OpenRCT2/OpenRCT2/wiki/Building-OpenRCT2-on-MSYS2-MinGW)

---

# 4. Contributing
OpenRCT2 uses the [gitflow workflow](https://www.atlassian.com/git/tutorials/comparing-workflows#gitflow-workflow). If you are implementing a new feature or fixing a bug, please branch off and perform pull requests to ```develop```. ```master``` only contains tagged releases, you should never branch off this.

Please read our [contributing guidelines](https://github.com/OpenRCT2/OpenRCT2/blob/develop/CONTRIBUTING.md) for information.

## 4.1 Bug fixes
A list of bugs can be found on the [issue tracker](https://github.com/OpenRCT2/OpenRCT2/issues). Feel free to work on any bug and submit a pull request to the develop branch with the fix. Mentioning that you intend to fix a bug on the issue will prevent other people from trying as well.

## 4.2 New features
Please talk to the OpenRCT2 team first before starting to develop a new feature. We may already have plans for or reasons against something that you'd like to work on. Therefore contacting us will allow us to help you or prevent you from wasting any time. You can talk to us via Discord, see links at the top of this page.

## 4.3 Translation
You can translate the game into other languages by editing the language files in ```data/language``` directory. Please join discussions in the [#localisation channel on Discord](https://discordapp.com/invite/sxnrvX9) and submit pull requests to [OpenRCT2/Localisation](https://github.com/OpenRCT2/Localisation).

## 4.4 Graphics
You can help create new graphics for the game by visiting the [OpenGraphics project](https://github.com/OpenRCT2/OpenGraphics). 3D modellers needed!

## 4.5 Audio
You can help create the music and sound effects for the game. Check out the [OpenMusic](https://github.com/OpenRCT2/OpenMusic) repository and drop by our [#open-sound-and-music channel on Discord](https://discord.gg/9y8WbcX) to find out more.

## 4.6 Scenarios
We would also like to distribute additional scenarios with the game, when the time comes. For that, we need talented scenario makers! Check out the [OpenScenarios repository](https://github.com/PFCKrutonium/OpenRCT2-OpenScenarios).

---

# 5. Policies

## 5.1 Code of Conduct

We have a [Code of Conduct](CODE_OF_CONDUCT.md) that applies to all OpenRCT2 projects. Please read it.

## 5.2 Code signing policy

We sign our releases with a digital certificate provided by SignPath Foundation.

Free code signing provided by [SignPath.io](https://about.signpath.io/), certificate by [SignPath Foundation](https://signpath.org/).

Signed releases can only be done by members of the [development team](https://github.com/OpenRCT2/OpenRCT2/blob/develop/contributors.md#development-team).

## 5.3 Privacy policy

See [PRIVACY.md](PRIVACY.md) for more information.

---

# 6. Licence
**OpenRCT2** is licensed under the GNU General Public License version 3 or (at your option) any later version. See the [`licence.txt`](licence.txt) file for more details.

---

# 7. More information
- [GitHub](https://github.com/OpenRCT2/OpenRCT2)
- [OpenRCT2.io](https://openrct2.io)
- [Facebook](https://www.facebook.com/OpenRCT2)
- [RCT subreddit](https://www.reddit.com/r/rct/)
- [OpenRCT2 subreddit](https://www.reddit.com/r/openrct2/)
- OpenRCT2 plug-ins
    - [Plug-in directory (unofficial)](https://openrct2plugins.org)
    - [Plug-in development documentation](https://github.com/OpenRCT2/OpenRCT2/blob/develop/distribution/scripting/scripting.md)

## Similar Projects

| [OpenLoco](https://github.com/OpenLoco/OpenLoco) | [OpenTTD](https://github.com/OpenTTD/OpenTTD) | [openage](https://github.com/SFTtech/openage) | [OpenRA](https://github.com/OpenRA/OpenRA) |
|:------------------------------------------------:|:----------------------------------------------------------------------------------------------------------:|:-------------------------------------------------------------------------------------------------------:|:-------------------------------------------------------------------------------------------------------------:|
| [![icon_x128](https://user-images.githubusercontent.com/604665/53047651-2c533c00-3493-11e9-911a-1a3540fc1156.png)](https://github.com/OpenLoco/OpenLoco) | [![](https://github.com/OpenTTD/OpenTTD/raw/850d05d24d4768c81d97765204ef2a487dd4972c/media/openttd.128.png)](https://github.com/OpenTTD/OpenTTD) | [![](https://user-images.githubusercontent.com/550290/36507534-4693f354-175a-11e8-93a7-faa0481474fb.png)](https://github.com/SFTtech/openage) | [![](https://raw.githubusercontent.com/OpenRA/OpenRA/bleed/packaging/artwork/ra_128x128.png)](https://github.com/OpenRA/OpenRA) |
| Chris Sawyer's Locomotion | Transport Tycoon Deluxe | Age of Empires 2 | Red Alert |

# 8. Sponsors

Companies that kindly allow us to use their stuff:

| [DigitalOcean](https://www.digitalocean.com/)                                                                                                                     | [JetBrains](https://www.jetbrains.com/)                                                                                                        | [Backtrace](https://backtrace.io/)                                                                                                        | [SignPath](https://signpath.org/)                                                                                  |
|-------------------------------------------------------------------------------------------------------------------------------------------------------------------|------------------------------------------------------------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------|--------------------------------------------------------------------------------------------------------------------|
| [![do_logo_vertical_blue svg](https://user-images.githubusercontent.com/550290/36508276-8b572f0e-175c-11e8-8622-9febbce756b2.png)](https://www.digitalocean.com/) | [![jetbrains](https://github.com/user-attachments/assets/0d1cf25e-706d-4e3a-96ee-157fbf2cf0c0)](https://www.jetbrains.com/) | [![backtrace](https://user-images.githubusercontent.com/550290/47113259-d0647680-d258-11e8-97c3-1a2c6bde6d11.png)](https://backtrace.io/) | [![Image](https://github.com/user-attachments/assets/2b5679e0-76a4-4ae7-bb37-a6a507a53466)](https://signpath.org/) |
| Hosting of various services                                                                                                                                       | CLion and other products                                                                                                                       | Minidump uploads and inspection                                                                                                           | Free code signing provided by [SignPath.io](https://about.signpath.io/), certificate by [SignPath Foundation](https://signpath.org/).                                                                                                       |
