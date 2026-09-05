# STEP 20 - Polish and Packaging

Priority: P2

## Settings

- YAML profile management and default profile selection.
- Default output directory.
- Recent STL documents.
- System appearance behavior.
- About screen with licenses and engine version.

## Packaging

- Use `xcodebuild archive` for Release builds.
- Sign with an explicit Developer ID identity when configured.
- Enable hardened runtime and document entitlements.
- Decide arm64-only versus universal output before release.
- Create a DMG with `hdiutil` only after signing.
- Notarize and staple the app when distribution credentials exist.
- Include reproducible version metadata.
- Include third-party license notices for Swift packages and engine dependencies.

## Acceptance

- A clean machine can install and launch the signed app.
- File import, RealityKit rendering, slicing, Cricut page export, and profile
  persistence work in a Release build.
