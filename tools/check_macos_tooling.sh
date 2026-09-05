#!/bin/sh
set -eu

failures=0
optional=0

if [ "${1:-}" = "--optional" ]; then
  optional=1
fi

report() {
  printf '%s\n' "$1"
}

require_command() {
  name=$1
  if command -v "$name" >/dev/null 2>&1; then
    report "[ok] $name: $(command -v "$name")"
  else
    report "[missing] $name"
    failures=$((failures + 1))
  fi
}

report "Layer Cut macOS tooling check"

if developer_dir=$(xcode-select -p 2>/dev/null); then
  report "[info] developer directory: $developer_dir"
else
  report "[missing] xcode-select developer directory"
  failures=$((failures + 1))
  developer_dir=""
fi

if command -v xcodebuild >/dev/null 2>&1 &&
   xcodebuild_version=$(xcodebuild -version 2>/dev/null); then
  report "[ok] xcodebuild: $(printf '%s\n' "$xcodebuild_version" | tr '\n' ' ')"
else
  report "[missing] full Xcode (Command Line Tools alone are insufficient)"
  report "       Install Xcode, launch it once, then run:"
  report "       sudo xcode-select --switch /Applications/Xcode.app/Contents/Developer"
  failures=$((failures + 1))
fi

if [ -n "$developer_dir" ] &&
   printf '%s' "$developer_dir" | grep -q '/Xcode.app/Contents/Developer'; then
  report "[ok] full Xcode developer directory selected"
elif [ -n "$developer_dir" ]; then
  report "[warn] selected developer directory is not full Xcode"
fi

if command -v xcrun >/dev/null 2>&1 &&
   sdk_path=$(xcrun --sdk macosx --show-sdk-path 2>/dev/null); then
  report "[ok] macOS SDK: $sdk_path"
else
  report "[missing] macOS SDK"
  failures=$((failures + 1))
fi

require_command swift
require_command clang++
require_command cmake
require_command ninja
require_command git
require_command codesign
require_command hdiutil

if [ "$optional" -eq 1 ]; then
  require_command security
  if command -v xcrun >/dev/null 2>&1 &&
     xcrun notarytool --help >/dev/null 2>&1; then
    report "[ok] xcrun notarytool"
  else
    report "[missing] xcrun notarytool"
    failures=$((failures + 1))
  fi
fi

if command -v cmake >/dev/null 2>&1; then
  cmake_version=$(cmake --version | sed -n '1p')
  report "[info] $cmake_version"
fi

if [ "$failures" -ne 0 ]; then
  report "Tooling check failed with $failures issue(s)."
  exit 1
fi

report "Tooling check passed."
