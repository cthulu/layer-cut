#!/usr/bin/env bash
set -Eeuo pipefail

# Install the tools required by the documented CMake build. The script is
# safe to run repeatedly; package managers skip already-installed packages.
case "$(uname -s)" in
  Darwin)
    if ! xcode-select -p >/dev/null 2>&1; then
      echo "Xcode Command Line Tools are required. Install them, then rerun this script." >&2
      xcode-select --install || true
      exit 1
    fi

    if ! command -v brew >/dev/null 2>&1; then
      echo "Installing Homebrew..."
      NONINTERACTIVE=1 /bin/bash -c \
        "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    fi

    eval "$(brew shellenv)"
    brew install cmake ninja pkg-config
    ;;
  Linux)
    if ! command -v apt-get >/dev/null 2>&1; then
      echo "This script supports macOS and Debian-based Linux only." >&2
      exit 1
    fi

    apt_prefix=()
    if [[ "$(id -u)" -ne 0 ]]; then
      command -v sudo >/dev/null 2>&1 || {
        echo "sudo is required to install Linux build dependencies." >&2
        exit 1
      }
      apt_prefix=(sudo)
    fi

    "${apt_prefix[@]}" apt-get update
    "${apt_prefix[@]}" apt-get install --yes \
      build-essential clang cmake ninja-build pkg-config curl ca-certificates
    ;;
  *)
    echo "Unsupported operating system: $(uname -s)" >&2
    exit 1
    ;;
esac

command -v clang++ >/dev/null 2>&1 || {
  echo "clang++ was not found after installation." >&2
  exit 1
}
command -v cmake >/dev/null 2>&1 || {
  echo "cmake was not found after installation." >&2
  exit 1
}
command -v ninja >/dev/null 2>&1 || {
  echo "ninja was not found after installation." >&2
  exit 1
}
command -v pkg-config >/dev/null 2>&1 || {
  echo "pkg-config was not found after installation." >&2
  exit 1
}

echo "Development environment ready."
echo "Build with: cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release"
echo "Test with:  ctest --test-dir build --output-on-failure"
