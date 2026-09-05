# STEP 12 - macOS Development Tooling

Priority: P0

## Required Tooling

- Full Xcode, not only Apple Command Line Tools.
- Xcode command-line tools selected with `xcode-select`.
- Swift toolchain matching the chosen Xcode release.
- Apple SDK for the minimum deployment target.
- CMake 3.20 or newer.
- Ninja, Git, Clang, `codesign`, and `hdiutil`.

Full Xcode is required for `xcodebuild`, SwiftUI/RealityKit SDK integration,
codesigning, and notarization tooling.

## Verification

Implement `tools/check_macos_tooling.sh`. It must use `set -eu`, print paths and
versions, require `xcodebuild`, reject a Command Line Tools-only developer
directory, check CMake 3.20+, Ninja, Swift, Clang, Git, `codesign`, `hdiutil`,
and resolve the active Xcode SDK. It must return nonzero for missing required
tools and print installation guidance without silently installing software.

Run:

```bash
tools/check_macos_tooling.sh
```

## Installation Guidance

1. Install full Xcode from the Mac App Store or Apple Developer downloads.
2. Launch Xcode once and accept the license/components prompt.
3. Run `sudo xcode-select --switch /Applications/Xcode.app/Contents/Developer`.
4. Run `sudo xcodebuild -license accept` if required by machine policy.
5. Install CMake and Ninja with Homebrew or the approved package manager.
6. Re-run the verification script.

Do not commit machine-specific Xcode paths or signing identities.
