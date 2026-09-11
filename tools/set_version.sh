#!/bin/bash
set -euo pipefail

# This script accepts one argument: the new version string (e.g., 0.2.1)
if [ -z "$1" ]; then
  echo "Usage: $0 <new_version>" >&2
  exit 1
fi

NEW_VERSION="$1"

echo "Setting project version to $NEW_VERSION..."

# 1. Update root CMakeLists.txt (CMakeLists.txt)
# Finds and replaces the PROJECT VERSION line.
sed -i '' "s/VERSION .*/VERSION $NEW_VERSION/" CMakeLists.txt
echo "Updated root CMake version in CMakeLists.txt."

echo "Version setting complete for $NEW_VERSION."
