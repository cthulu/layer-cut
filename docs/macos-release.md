# macOS Release Packaging

The package script requires macOS 14 or later, full Xcode, CMake 3.20 or
later, and the Xcode command-line tools. Verify the toolchain first:

```bash
./tools/check_macos_tooling.sh
```

Create an unsigned local release package with the default `arm64` architecture:

```bash
./tools/package_macos_app.sh
```

The script writes all release artifacts to `dist/` by default:

- DMG: `dist/LayerCut-0.1.0-arm64.dmg`
- Xcode archive: `dist/LayerCut.xcarchive`

The script prints the exact paths at the end of a successful run. Set
`LAYER_CUT_VERSION` to change the version, or `LAYER_CUT_RELEASE_DIR` to use a
different output directory:

```bash
LAYER_CUT_VERSION=1.0.0 \
LAYER_CUT_RELEASE_DIR="$HOME/Desktop/layer-cut-release" \
./tools/package_macos_app.sh
```

The DMG will then be at:

```text
$HOME/Desktop/layer-cut-release/LayerCut-1.0.0-arm64.dmg
```

To create a universal package, provide a comma-separated architecture list:

```bash
MACOS_ARCHS=arm64,x86_64 ./tools/package_macos_app.sh
```

For signed distribution builds, set `DEVELOPER_ID_APPLICATION` and
`DEVELOPMENT_TEAM`. To submit the DMG for notarization and staple the result,
also set `NOTARY_PROFILE` to an existing `notarytool` keychain profile.

The app uses the hardened runtime. Local builds use an ad-hoc signature when
no Developer ID identity is supplied. No Apple credentials are required for
local builds or tests.
