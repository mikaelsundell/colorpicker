#!/bin/bash
##  Copyright 2022-present Contributors to the colorpicker project.
##  SPDX-License-Identifier: BSD-3-Clause
##  https://github.com/mikaelsundell/colorpicker

# usage
if [ "$#" -ne 1 ]; then
  echo "Usage: $0 <path_to_1024x1024_png_file>"
  exit 1
fi

inputfile="$1"

if [ ! -f "$inputfile" ]; then
  echo "File not found: $inputfile"
  exit 1
fi

basename=$(basename "$inputfile" .png)

iconsetdir="${basename}.iconset"
mkdir -p "$iconsetdir"

sips -z 16 16     "$inputfile" --out "$iconsetdir/icon_16x16.png"
sips -z 32 32     "$inputfile" --out "$iconsetdir/icon_16x16@2x.png"
sips -z 32 32     "$inputfile" --out "$iconsetdir/icon_32x32.png"
sips -z 64 64     "$inputfile" --out "$iconsetdir/icon_32x32@2x.png"
sips -z 128 128   "$inputfile" --out "$iconsetdir/icon_128x128.png"
sips -z 256 256   "$inputfile" --out "$iconsetdir/icon_128x128@2x.png"
sips -z 256 256   "$inputfile" --out "$iconsetdir/icon_256x256.png"
sips -z 512 512   "$inputfile" --out "$iconsetdir/icon_256x256@2x.png"
sips -z 512 512   "$inputfile" --out "$iconsetdir/icon_512x512.png"
cp "$inputfile"  "$iconsetdir/icon_512x512@2x.png"
iconutil -c icns "$iconsetdir"

if [ -f "${basename}.icns" ]; then
  echo "${basename}.icns created successfully"
else
  echo "Failed to create ${basename}.icns"
fi

rm -rf "$iconsetdir"
