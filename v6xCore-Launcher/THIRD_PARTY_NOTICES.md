# Third-party notices — v6xCore Launcher 3.0

## Dear ImGui

Dear ImGui 1.91.9b, commit `f5befd2d29e66809cd1110a152e375a7f1981f06`.
Source: https://github.com/ocornut/imgui
MIT license: `vendor/imgui/LICENSE.txt`.

## nlohmann/json

Version 3.11.3, https://github.com/nlohmann/json
MIT license: `vendor/nlohmann/LICENSE.MIT`.

## Monocypher

Version 4.0.2, commit `0d85f98c9d9b0227e42cf795cb527dff372b40a4`.
Source: https://github.com/LoupVaillant/Monocypher
Original license terms, including the BSD-2-Clause / CC0 licensing options, are preserved in `vendor/monocypher/LICENCE.md`.
Used for standard Ed25519 response verification. No third-party prebuilt crypto library is executed or bundled.

## Poppins

Source: https://github.com/google/fonts/tree/main/ofl/poppins
SIL Open Font License 1.1: `assets/OFL.txt`.
Original Regular and Medium font files are embedded without modification.

## KeyAuth protocol

This project implements a small independent HTTPS client for the public KeyAuth 1.3 protocol. The official C++ and Python examples were reviewed for request fields, paid-session checking, response headers, and the published signature verification key. Their example/SDK implementations, security watchdogs and prebuilt libraries are not redistributed.

References: https://github.com/KeyAuth/keyauth-cpp-library and https://github.com/KeyAuth/KeyAuth-Python-Example

Only public application identifiers are embedded. The supplied application secret was not included.

## Offset/schema facts

Data source: https://github.com/sezzyaep/CS2-OFFSETS
The package retains the required numeric offsets, field identifiers and type metadata, not that repository's dumper implementation. No repository license was declared at inspection, and no MIT-license claim is made for it.

`reference/provenance.json` records the source commit, original download hashes and extracted-data hashes. Legacy a2x notices are retained for earlier reference material, not applied to the new source.

## Compiler runtimes

MinGW-W64/GCC runtime notices and applicable runtime-exception information are preserved in `reference/toolchain-licenses/`.

## Artwork and supplied logo

`assets/v6x_loading.gif` is the user's supplied, unchanged animation. The icon and animation atlas derive from it. The application license does not grant ownership of that supplied artwork.

The eight monochrome game-library covers are original AI-generated, genre-inspired illustrations prepared for this interface. They are not official game promotional artwork. Game names identify the catalog entries; this project does not claim endorsement by their publishers.

The Sweetie repository and supplied loader screenshots were visual references only. No custom Sweetie widgets, wallpaper, font arrays or binaries are redistributed.

Dependency/font notices are also embedded and written to `licenses/THIRD_PARTY_LICENSES.txt` when missing. Application source uses the project `LICENSE`; third-party components and supplied artwork retain their own terms.
