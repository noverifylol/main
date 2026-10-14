# Build and verification report — v6xCore Launcher 3.0

Date: 2026-10-14T05:41:42+02:00

## Delivered artifact

Native Windows x64 EXE, optimized/stripped MinGW release, Win32 + DirectX 11 + Dear ImGui. Embedded monochrome artwork, interpolated logo, fonts, startup data and dependency notices.

- File: `v6xCore Launcher.exe`
- Size: 5,111,808 bytes
- SHA-256: `5df0ced58228b2b538bf86928410da1a1a256f48573968f327811778477ab73f`
- MD5 for the optional KeyAuth executable-hash setting: `219d170b24d19666ab8481c49a775d6e`

The separately named `v6xCore-Launcher-3.0.exe` is byte-identical. It is not Authenticode-signed. Response-signature verification must not be confused with executable code signing.

## Executed checks

| Check | Result |
|---|---|
| Windows x64 cross-build | Passed |
| Own application code with `-Wall -Wextra -Wpedantic -Werror` | Passed |
| Math/projection/input-scaling unit tests | Passed |
| Safe config-name tests | Passed |
| Offset/INI data-format C++ tests | Passed |
| FFA/team-selection/deadline C++ tests | Passed |
| Ed25519 protocol tests | Passed |
| Python offset tests | 6/6 passed |
| Win32 settings tests under Wine | Passed |
| Embedded data/bootstrap/Unicode tests under Wine | Passed |
| DPAPI credential-cache tests under Wine | Passed |
| Actual target-selection tests without process access/input | Passed |
| Final standalone EXE in a Unicode folder, unrelated working directory | Passed |
| Real KeyAuth public initialization from final Windows EXE | Passed |
| Eight covers, sign-in, library/search, settings and CS2 tabs | Rendered in the native EXE |
| FFA/teammate switches and 360 Hz target settings | Changed through UI and persisted |
| Config save/load/delete with confirmation and default restore | Passed |
| Capture-privacy request | Executed; **not confirmed**, Windows error 8 under Wine |
| Final indexed logo texture | Decoded and visibly advanced |
| Normal GUI shutdown after final check | Exit code 0 |

## Authentication scope

A real HTTPS initialization response for the supplied public CS2 application settings was verified using the published Ed25519 key, timestamp freshness, owner and success-code checks. No application secret or customer license was sent by these public-initialization checks.

Synthetic protocol tests cover a valid test signature, rejection by the production verification key, altered bodies, stale/future timestamps, wrong owners, invalid success codes/types, expired subscriptions, form encoding, and credential-safe error messages.

Windows DPAPI tests cover round-trip protection, absence of the synthetic key in the saved bytes, game/application namespace binding, tampered-ciphertext rejection, deletion, and app-scoped identity derivation. Test keys were synthetic, local-only, and never sent to KeyAuth.

**No valid customer key was provided for end-to-end paid sign-in, expiry/revocation, or 2FA testing. Those live paths remain unverified.** The UI preview does not grant a paid entitlement and cannot read CS2 or emit aim input.

## UI observations

The real Windows executable was rendered and operated under Wine 10 / Xvfb, with `LC_ALL=C.UTF-8`. Images are native-application captures, not browser mockups. Preview screens are explicitly labeled.

A dropdown-navigation bug was found and fixed: the module's End-to-exit hotkey is now suppressed while editing widgets or using popups. Config CRUD, FFA/teammate settings, sampling/render targets, library filters/search, Coming soon dialogs, and capture-privacy preference persistence were exercised.

One final Wine launch emitted a page-fault/debugger message before `V6X_UI_READY`. The application subsequently rendered, its logo animation advanced, it remained interactive through library/module checks, and it closed with exit code 0. This observation is retained here rather than presented as a clean native-Windows stability test.

## Important untested areas

- A physical/native Windows system.
- Real customer-license authentication and paid-session/2FA behavior.
- Actual CS2 gameplay, live overlay behavior or aim accuracy.
- Achievable sampling/overlay rates over CS2 on the user's hardware.
- Capture exclusion on actual Windows capture tools or external capture hardware.
- Anti-cheat compatibility, detection status, or account safety.
- The supplied MSVC and Windows CMake build paths.

The application reports capture exclusion as unconfirmed when its API checks fail. It is not marketed as universally streamproof or uncrackable.

## Binary inspection

Verified PE flags: **ASLR/DYNAMIC_BASE**, **DEP/NX_COMPAT**, **HIGH_ENTROPY_VA**.

Imported system libraries: ADVAPI32.dll, bcrypt.dll, CRYPT32.dll, d3d11.dll, D3DCOMPILER_47.dll, dwmapi.dll, GDI32.dll, gdiplus.dll, KERNEL32.dll, msvcrt.dll, ole32.dll, SHELL32.dll, USER32.dll, WINHTTP.dll.

Normal user-mode APIs include `ReadProcessMemory`, `SendInput`, `CryptProtectData`, `CryptUnprotectData`, and `SetWindowDisplayAffinity`. No `WriteProcessMemory`, `CreateRemoteThread`, `VirtualAllocEx`, `DeviceIoControl`, or `CreateService` import was found. No kernel driver or covert executable payload is supplied.

This is an import/flag inspection, not a security audit or anti-cheat assurance. The compiler noted reduced column tracking from large headers; the build succeeded, but that is not a proof of general bug-freedom.

## Reproduction and integrity

- `bash tools/cross-build.sh`
- `tests/test_license_protocol.cpp`
- `tests/test_license_cache.cpp`
- `tests/test_team_rules.cpp`
- `tests/test_target_selection.cpp`
- `tests/ui_smoke_linux.py` — requires an already running `--ui-test` instance in a disposable copy.
- `tests/release_check_linux.py` — local-only standalone bootstrap regression.
- `tests/ui-v3-result.json` — recorded UI checks and limitations.

`MANIFEST.sha256` covers every packaged file except itself. User changes to configs, preferences or offsets naturally change their hashes. The original supplied GIF was byte-compared and remains unchanged.
