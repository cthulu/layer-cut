#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BUILD_DIR="$ROOT_DIR/build/macos-xcode"
PROJECT="$BUILD_DIR/layer-cut.xcodeproj"
TMP_ROOT=${TMPDIR:-/tmp}
DERIVED_DATA_DIR=$(mktemp -d "${TMP_ROOT%/}/layer-cut-xcode.XXXXXX")

cleanup() {
  rm -rf "$DERIVED_DATA_DIR"
}
trap cleanup EXIT HUP INT TERM

"$ROOT_DIR/tools/check_macos_tooling.sh"

cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -G Xcode \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DLAYER_CUT_BUILD_MACOS_APP=ON \
  -DLAYER_CUT_FETCH_DEPENDENCIES=OFF \
  -DBUILD_TESTING=OFF

xcodebuild \
  -project "$PROJECT" \
  -scheme LayerCutApp \
  -configuration Debug \
  -derivedDataPath "$DERIVED_DATA_DIR" \
  ARCHS=arm64 \
  ONLY_ACTIVE_ARCH=NO \
  CODE_SIGNING_ALLOWED=NO \
  build
