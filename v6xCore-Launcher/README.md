# v6xCore Launcher 3.0 — Mono Edition

**Native Windows x64 · English interface · KeyAuth 1.3 · CS2 user-mode module**

An independently built black-and-white launcher with the supplied v6x animation, eight illustrated game covers, license sign-in, a searchable library, workspace settings, and the integrated CS2 control panel.

## What is available

| Game | Status |
|---|---|
| Counter-Strike 2 | Integrated external client; requires a verified customer license |
| Fortnite | Coming soon — artwork and catalog entry only |
| Project Retrac | Coming soon — artwork and catalog entry only |
| Valorant | Coming soon — artwork and catalog entry only |
| Roblox | Coming soon — artwork and catalog entry only |
| Rainbow Six Siege | Coming soon — artwork and catalog entry only |
| Call of Duty: Warzone | Coming soon — artwork and catalog entry only |
| PUBG: Battlegrounds | Coming soon — artwork and catalog entry only |

**Launch** opens the integrated external CS2 module. It is not an injector. No driver, exploit/bypass code, concealed executable payload, silent bullet redirection, forced-hit manipulation, prediction, or recoil rewriting is included. The launcher does not claim any module is “undetected.”

## Quick start

1. Close any older v6xCore instance.
2. Put `v6xCore Launcher.exe` in its own folder, or extract this ZIP into a new folder.
3. Start the EXE. Required defaults, configs, artwork, fonts, and dependency notices are embedded; no installer or Python is needed for normal use.
4. Enter a **customer license** created for the configured Counter Strike 2 KeyAuth application. Do not enter an application secret. If the account requires it, expand the 2FA field.
5. A verified license opens the library. Choose **Launch** on the Counter-Strike 2 card.
6. Run CS2 in windowed or borderless mode. The module can request normal read access to `cs2.exe`; protected access is not bypassed.
7. Aim control starts off and requires deliberate activation plus a held key. Ingame output also requires valid data and foreground CS2 focus.

The EXE is unsigned. No code-signing certificate was supplied. A successful build, sign-in, or offset-file check does not establish compatibility with an installed CS2 build or anti-cheat system. Public matchmaking can result in an account restriction.

## KeyAuth integration

The current API uses public application identifiers and signed server responses, not a client-embedded application secret.

Configured in `src/catalog.hpp`:

```text
Application: Counter Strike 2
Owner: 2goY8wtVEd
KeyAuth application version: 1.0
Client API: https://keyauth.win/api/1.3/
```

The launcher file version is **3.0**; the separately configured KeyAuth application version stays **1.0** as requested.

- HTTPS certificate validation remains enabled.
- Ed25519 signatures are checked using KeyAuth's published verification key before a response can grant access.
- Timestamp freshness, owner identity, success type/code, and active subscription expiry are checked.
- Paid-session checks run periodically. The module stops output when authorization becomes invalid or cannot be revalidated; a stale locally cached key is not treated as an offline entitlement.
- Initial connection, missing key, invalid/expired license, identity binding, 2FA, service, clock, and signature failures have English error paths. Errors do not print submitted keys, session IDs, or raw authentication bodies.
- The client sends an app-scoped SHA-256 identifier derived from the current Windows user SID for license binding. Existing licenses bound using another client's identifier may require an owner-managed reset.
- KeyAuth receives the submitted license over HTTPS, the public app configuration, executable checksum, and the scoped identity. The network service can also observe the requesting IP address. No gameplay data is uploaded.

**Rotate the application secret previously shared in chat.** It was deliberately not copied into the source, EXE, or logs. Changing that secret does not require embedding the replacement in this client.

### Remembered licenses

Selecting **Remember this license on this PC** saves the validated key with Windows **DPAPI, CurrentUser**, under `%LOCALAPPDATA%\v6xCore\licenses`.

Storage names and encryption entropy are separated by game/application/owner. A saved key is decrypted only for online revalidation; it is not sufficient to bypass licensing. Two-factor authentication can still require a new code. Copying the encrypted file to another Windows account is not a supported migration method.

Workspace settings include **Forget saved license and sign out**. Turning off “Remember future sign-ins” stops future automatic use but does not itself erase an existing encrypted file; use the explicit forget action to remove it.

Only the CS2 application has been configured. The other seven entries have independent, empty configuration slots; they do not reuse the CS2 application's credentials or pretend to authenticate an unreleased module.

## CS2 changes

### FFA and teammates

Under **Aim control → Player relations**, select **Team matches** or **Free-for-all**. This is an explicit mode choice, not an unverified automatic game-mode detector.

- Team mode respects the game's team IDs.
- FFA treats all other valid players as opponents, even when the game still assigns the same team number. It uses opponent colors and no longer hides those players behind the teammate display filter.
- **Target teammates** is a separate, opt-in team-mode setting. In FFA it is unnecessary and disabled.
- Self and immune players remain excluded by the reader/target-selection rules. The feature does not change friendly-fire or server rules.

### Configs

The tab, actions, and active folder are now **Configs** / `configs/`.

Save, load, overwrite, delete with confirmation, reset, and open-folder actions remain functional. Existing safe `.ini` files in a legacy `profiles/` directory are copied into `configs/` without overwriting matching names or deleting originals. Aim activation is never saved or automatically restored.

### Refresh scheduling

- Sampling targets: **60 / 120 / 165 / 240 / 360 Hz**.
- Overlay targets: **30 / 60 / 120 / 165 / 240 / 360 Hz**.
- Launcher/interface targets: **60 / 90 / 120 / 144 Hz**.
- UI rendering and sampling use separate deadlines; the UI no longer intentionally waits for VSync on every sampling iteration.
- The overlay retains its backbuffer instead of reallocating a full bitmap every paint.
- The System tab and optional watermark show measured sampling activity, not a made-up guaranteed rate.

These are **targets**, limited by read cost, scheduling, graphics, the monitor, and system load. Faster polling does not raise the game's tick rate or guarantee hits.

### Input and animation

Manual pitch/yaw inputs were removed from the menu. Input scaling uses the read sensitivity and scoped adjustment while assuming the normal CS2 axis scales. It does not compensate for arbitrary modified console axis settings, spread, recoil, or server-side shot outcomes.

The supplied GIF remains unchanged in `assets/v6x_loading.gif`. The UI atlas now has **672 interpolated samples across the original 5.6-second loop**, compared with 168 source frames. The prepared monochrome atlas is compact and embedded. Actual animation cadence depends on the selected interface target and rendering performance.

## Capture privacy

**Settings → Capture privacy (best effort)** requests `WDA_EXCLUDEFROMCAPTURE` for the launcher's own window and its own overlay. The UI reports whether the API call and readback were accepted.

This is not universal “streamproof” behavior, DRM, or an anti-cheat bypass. Windows version, desktop composition, and capture method affect the result; cameras, external capture hardware, or other methods may still capture content. The native capture-exclusion behavior could not be confirmed under Wine: the tested environment returned **Windows error 8**, and the UI showed **Not confirmed**.

Test your actual recording setup before relying on this option. Reference: https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-setwindowdisplayaffinity

## Hotkeys

Inside the active CS2 module: `F1` ESP, `F2` aim activation, `F3` boxes, `F4` names, `F5` health bar, `F6` teammate display, `F7` FOV ring, `F8` radar-known filter, `F9` reload/disarm, `Pause` pause, `Insert` minimize/restore, `End` exit.

Hotkeys are suspended during menu text editing, active widget interaction, and popups. This also prevents `End` used for dropdown navigation from closing the application.

## Data and diagnostics

Startup files normally live beside the EXE. A non-writable EXE directory falls back to `%LOCALAPPDATA%\v6xCore\Cs2External`. The System tab opens the real data folder and exports UTF-8 diagnostics.

Offsets remain pinned to `sezzyaep/CS2-OFFSETS`, build **14190**, commit `fa24ed457f096f519d2cdbb95db55f4b7bc5c6df` from **9 October 2026**. The in-app native updater validates new JSON from one source commit before replacing data and retains a backup. Successful format validation is not proof of live-game compatibility.

```powershell
& '.\v6xCore Launcher.exe' --self-check
& '.\v6xCore Launcher.exe' --auth-check
& '.\v6xCore Launcher.exe' --update-offsets
& '.\v6xCore Launcher.exe' --repair-offsets
```

Close a running GUI instance first. `--auth-check` performs public application initialization and signature verification only; it neither sends a customer license nor verifies paid access.

### UI preview without a key

```powershell
& '.\v6xCore Launcher.exe' --ui-test
```

Then choose **Preview library**. This mode is explicitly labeled, makes no authentication requests, does not load remembered keys, and cannot connect to CS2 or emit aim input. It is not an authentication bypass for the working module.

## Build and protection

Source remains readable. The supplied MinGW release is optimized and stripped, with **ASLR, DEP/NX and high-entropy VA** flags. Credentials are encrypted at rest, and authentication responses are signature-checked.

The executable itself is **not an encrypted payload**. No client binary can honestly be promised “uncrackable,” especially when its source is available. This build contains no anti-debugging, covert loading, in-memory executable deployment, or anti-cheat evasion routines.

Windows: Visual Studio 2022 x64 Native Tools / C++ desktop workload and Windows SDK:

```bat
build.cmd
```

CMake 3.20+:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The executed build path is Linux/MinGW-W64:

```bash
bash tools/cross-build.sh
```

MSVC and CMake definitions are supplied, but those Windows build paths were not run here. Dependencies, fonts, covers, and prepared logo resources are included. Regenerating the logo is optional: run `tools/prepare_logo.py`, then `tools/smooth_logo.py`; this needs Pillow, NumPy, and OpenCV.

## Verification limits

See `BUILD_REPORT.md` for exact results. Native Windows binaries were built and tested under Wine. Public KeyAuth initialization and its signature were verified against the real service; DPAPI storage and protocol/targeting tests passed. GUI navigation and Configs CRUD were exercised.

**A real customer-license login, actual CS2 gameplay, native Windows capture exclusion, and anti-cheat compatibility were not tested.** The screenshots are native-EXE captures; library/module screenshots marked preview are not evidence of a paid login or live gameplay.
