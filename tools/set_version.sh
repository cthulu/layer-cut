#!/bin/bash
set -euo pipefail

# This script accepts one argument: the new version string (e.g., 0.2.0)
if [ -z "$1" ]; then
  echo "Usage: $0 <new_version>" >&2
  exit 1
fi

NEW_VERSION="$1"

echo "Setting project version to $NEW_VERSION..."

# 1. Update root CMakeLists.txt (layer-cut/CMakeLists.txt)
# Finds and replaces the PROJECT VERSION line.
sed -i "s/VERSION.*/VERSION $NEW_VERSION/" layer-cut/CMakeLists.txt
echo "Updated root CMake version in layer-cut/CMakeLists.txt."

# 2. Update macos-app/CMakeLists.txt
# Sets the macro variable LAYER_CUT_VERSION used by the application build system.
sed -i "s|LAYER_CUT_VERSION=.*|LAYER_CUT_VERSION=$NEW_VERSION|" macos-app/CMakeLists.txt
echo "Updated app CMake version variable in macos-app/CMakeLists.txt."

# Add other targets here as needed (e.g., README.md, tags)

echo "Version setting complete for $NEW_VERSION."