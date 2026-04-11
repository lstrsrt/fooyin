#!/bin/sh

set -eu

FOOYIN_DIR=$PWD
BUILD_DIR=build
PACKAGE_DIR=arch
STAGE_DIR=pkg

cmake -S "$FOOYIN_DIR" \
  -G Ninja \
  -B "$BUILD_DIR" \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON

cmake --build "$BUILD_DIR"

mkdir -p "$PACKAGE_DIR"
rm -rf "$STAGE_DIR"

DESTDIR="$PWD/$STAGE_DIR" cmake --install "$BUILD_DIR" --prefix /usr --strip

ARCH=$(uname -m)
VERSION=$(sed -n 's/^[[:space:]]*VERSION[[:space:]]\+\([0-9][0-9.]*\).*/\1/p' "$FOOYIN_DIR/CMakeLists.txt" | head -n 1)

if [ -z "$VERSION" ]; then
  echo "Failed to determine fooyin version" >&2
  exit 1
fi

ARCHIVE="fooyin-${VERSION}-archlinux-${ARCH}.tar.zst"

tar --zstd -cf "$PACKAGE_DIR/$ARCHIVE" -C "$STAGE_DIR" .
