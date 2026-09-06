#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BUILD_DIR=${LAYER_CUT_BUILD_DIR:-"$ROOT_DIR/build/cli"}

cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DLAYER_CUT_FETCH_DEPENDENCIES=ON \
  -DBUILD_TESTING=OFF

cmake --build "$BUILD_DIR" --target layer-cut
printf 'CLI available at: %s\n' "$BUILD_DIR/cli/layer-cut"
