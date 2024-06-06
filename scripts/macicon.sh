#!/bin/bash
##  macicon.sh
##  Colorman
##
##  Copyright (c) 2023 - present Mikael Sundell.
##  All Rights Reserved.

# usage

if [ "$#" -ne 1 ]; then
  echo "Usage: $0 <path_to_1024x1024_png_file>"
  exit 1
fi

INPUT_FILE="$1"

if [ ! -f "$INPUT_FILE" ]; then
  echo "File not found: $INPUT_FILE"
  exit 1
fi

BASE_NAME=$(basename "$INPUT_FILE" .png)

ICONSET_DIR="${BASE_NAME}.iconset"
mkdir -p "$ICONSET_DIR"

# Generate the various icon sizes
sips -z 16 16     "$INPUT_FILE" --out "$ICONSET_DIR/icon_16x16.png"
sips -z 32 32     "$INPUT_FILE" --out "$ICONSET_DIR/icon_16x16@2x.png"
sips -z 32 32     "$INPUT_FILE" --out "$ICONSET_DIR/icon_32x32.png"
sips -z 64 64     "$INPUT_FILE" --out "$ICONSET_DIR/icon_32x32@2x.png"
sips -z 128 128   "$INPUT_FILE" --out "$ICONSET_DIR/icon_128x128.png"
sips -z 256 256   "$INPUT_FILE" --out "$ICONSET_DIR/icon_128x128@2x.png"
sips -z 256 256   "$INPUT_FILE" --out "$ICONSET_DIR/icon_256x256.png"
sips -z 512 512   "$INPUT_FILE" --out "$ICONSET_DIR/icon_256x256@2x.png"
sips -z 512 512   "$INPUT_FILE" --out "$ICONSET_DIR/icon_512x512.png"
cp "$INPUT_FILE"  "$ICONSET_DIR/icon_512x512@2x.png"

iconutil -c icns "$ICONSET_DIR"

if [ -f "${BASE_NAME}.icns" ]; then
  echo "${BASE_NAME}.icns created successfully"
else
  echo "Failed to create ${BASE_NAME}.icns"
fi

rm -rf "$ICONSET_DIR"
