#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
VERSION=${LAYER_CUT_VERSION:-0.1.0}
ARCHS=${MACOS_ARCHS:-arm64}
# Accept the conventional comma-separated environment value, but pass the
# format expected by each build tool.
XCODE_ARCHS=$(printf '%s' "$ARCHS" | tr ',' ' ')
CMAKE_ARCHS=$(printf '%s' "$XCODE_ARCHS" | tr ' ' ';')
IDENTITY=${DEVELOPER_ID_APPLICATION:-}
NOTARY_PROFILE=${NOTARY_PROFILE:-}
OUT_DIR=${LAYER_CUT_RELEASE_DIR:-"$ROOT_DIR/dist"}
BUILD_DIR="$ROOT_DIR/build/macos-release"
ARCHIVE_PATH="$OUT_DIR/LayerCut.xcarchive"
APP_PATH="$ARCHIVE_PATH/Products/Applications/LayerCut.app"
BUILD_APP_PATH="$BUILD_DIR/macos-app/Release/LayerCut.app"
DMG_PATH="$OUT_DIR/LayerCut-$VERSION-$ARCHS.dmg"

mkdir -p "$OUT_DIR"
"$ROOT_DIR/tools/check_macos_tooling.sh"
cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -G Xcode -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 -DCMAKE_OSX_ARCHITECTURES="$CMAKE_ARCHS" \
  -DLAYER_CUT_BUILD_MACOS_APP=ON -DLAYER_CUT_FETCH_DEPENDENCIES=ON -DBUILD_TESTING=OFF

rm -rf "$ARCHIVE_PATH"
SIGNING_ARGS="CODE_SIGNING_ALLOWED=YES CODE_SIGNING_REQUIRED=NO CODE_SIGN_IDENTITY=-"
if [ -n "$IDENTITY" ]; then
  SIGNING_ARGS="CODE_SIGNING_ALLOWED=YES CODE_SIGNING_REQUIRED=YES CODE_SIGN_IDENTITY=$IDENTITY DEVELOPMENT_TEAM=${DEVELOPMENT_TEAM:-}"
fi

# Never create the DMG before this archive and signature verification succeed.
xcodebuild -project "$BUILD_DIR/layer-cut.xcodeproj" -scheme LayerCutApp -configuration Release \
  -archivePath "$ARCHIVE_PATH" ARCHS="$XCODE_ARCHS" ONLY_ACTIVE_ARCH=NO MARKETING_VERSION="$VERSION" \
  $SIGNING_ARGS archive

# CMake-generated Xcode projects may archive the app as an uninstalled
# product. Populate the archive explicitly so the packaging steps do not
# depend on Xcode's install-product behavior.
if [ ! -d "$APP_PATH" ] && [ -d "$BUILD_APP_PATH" ]; then
  mkdir -p "$(dirname -- "$APP_PATH")"
  cp -R -L "$BUILD_APP_PATH" "$APP_PATH"
fi
test -d "$APP_PATH"
codesign --verify --deep --strict "$APP_PATH"

STAGING=$(mktemp -d "${TMPDIR:-/tmp}/layer-cut-dmg.XXXXXX")
trap 'rm -rf "$STAGING"' EXIT HUP INT TERM
cp -R -L "$APP_PATH" "$STAGING/Layer Cut.app"
ln -s /Applications "$STAGING/Applications"
rm -f "$DMG_PATH"
hdiutil create -volname "Layer Cut $VERSION" -srcfolder "$STAGING" -ov -format UDZO "$DMG_PATH"

if [ -n "$NOTARY_PROFILE" ]; then
  xcrun notarytool submit "$DMG_PATH" --keychain-profile "$NOTARY_PROFILE" --wait
  xcrun stapler staple "$APP_PATH"
  codesign --verify --deep --strict "$APP_PATH"
fi

printf '%s\n' "Archive: $ARCHIVE_PATH" "DMG: $DMG_PATH" "Version: $VERSION" "Architectures: $ARCHS"
