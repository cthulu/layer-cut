# macOS Release Packaging

`tools/package_macos_app.sh` configures a Release Xcode project, runs
`xcodebuild archive`, verifies the archived app, and only then creates a DMG.

The first release is `arm64` only. A universal build is opt-in:

```bash
MACOS_ARCHS=arm64,x86_64 LAYER_CUT_VERSION=0.1.0 ./tools/package_macos_app.sh
```

Set `DEVELOPER_ID_APPLICATION` to an explicit Developer ID Application
identity for distribution signing. Without it, local packaging uses an
ad-hoc signature. Set `DEVELOPMENT_TEAM` when required by the identity.

The app enables hardened runtime. `macos-app/LayerCut.entitlements` is empty:
file import/export uses user-selected locations and no network, JIT, or private
capability is required. Any future entitlement must be reviewed before release.

Notarization is optional. Set `NOTARY_PROFILE` to a stored `notarytool`
keychain profile to submit the DMG, wait for approval, and staple the archive.
No Apple credentials are required for local builds or tests.

`LAYER_CUT_VERSION` is used for bundle marketing metadata and the DMG name.
It should match the repository CMake project version; artifacts do not contain
wall-clock metadata.
