# <img src="resources/AppIcon.png" valign="middle" alt="Icon" width="50" height="50"> Colorpicker #

[![License](https://img.shields.io/badge/license-BSD%203--Clause-blue.svg?style=flat-square)](https://github.com/mikaelsundell/icloud-snapshot/blob/master/license.md)

Introduction
------------

<img src="https://github.com/mikaelsundell/colorpicker/blob/1230ca62c7c94554b4c59d70b181ba1c5f4302ee/resources/Colorpicker.png" />

Colorpicker is a versatile Mac application designed to select and capture colors from various screens. It features a color wheel visualizer, aiding users in color design by offering tools to create harmonious color palettes and explore color relationships.

|  Download        | Description |
| ----------------| ----------- |
|  [<img src="resources/Download.png" valign="middle" alt="Icon" width="16" height="16"> Colorman v0.0.10](https://github.com/mikaelsundell/colorpicker/releases/download/release-v0.0.10/Colorpicker_macOS12_arm64_release.dmg) | [Apple Silicon macOS12+](https://github.com/mikaelsundell/colorpicker/releases/download/release-v0.0.10/Colorpicker_macOS12_arm64_release.dmg)


Documentation
-------------

**How to use**

Colorpicker needs to be active to pick colors. The app is always active when launched but can be activated using `View > Active [A]`. Once active the application will show the current color interactivly in a hsl color wheel. Use the color picker button or `Edit > Pick [P]` to start. Colors will start to appear on the color wheel. To end the color pick tool use either `Right mouse button` or `Esc` to end. After color pick app will no longer be active, colors can now be checked by clicking on the in the color wheel.

**Display profile**

The default native display values are the native colors from the display. The color values equals a screenshot with the display profile .icc applied. Switch between different display profiles to display color values in different color spaces, transformed from the display profile .icc.

**Color processing in LCMS**

LCMS is a popular color management system for use in open source projects. LCMS closely matches ColorSync, AdobeACE, Reference ICC and other color engines. There may be variations, different engines should roughly match.

Privacy & Security
------------------

colorpicker uses macOS to create images from display and needs permission for `Screen recording`. Add Colorpicker app to the list of allowed applications in `System settings`.

Build & Package
------------------
Colorpicker is built using the ```build.sh``` script.

Build all - debug and release:
```shell
./build.sh all
```

Requires 3rdparty build at the same level as Colorpicker.

Colorpicker Advanced
--------

The Colorpicker app can be built both from commandline or using Xcode `-GXcode`. Use `DYLD_IMAGE_SUFFIX=_debug` environment variable to link and run with Qt debug frameworks.

## CMake configuration ##

Add cmake to path:

```shell
export PATH=$PATH:/Applications/CMake.app/Contents/bin
```

## Build configuration ##

```shell
mkdir build
cd build
cmake .. -DCMAKE_MODULE_PATH=<path>/colorpicker/modules -DCMAKE_PREFIX_PATH=<path>/3rdparty/build/macosx/arm64.debug -GXcode
cmake --build . --config Release --parallel
```

**Packaging**

The `macdeploy.sh` script will deploy mac bundle including dependencies.

```shell
./macdeploy.sh -b <path>/Colorpicker.app -m <path>/macdeployqt
```

The `macdmg.sh` script will create a application disk image.

```shell
./macdmg.sh -d <path>/Colorpicker_macOS12_<arch>.dmg
```

Web Resources
-------------

* GitHub page:        https://github.com/mikaelsundell/colorpicker
* Issues              https://github.com/mikaelsundell/colorpicker/issues

Copyright
---------

**3rdparty libraries acknowledgment and copyright notice**

This product includes software developed by third parties. The copyrights and terms of use of these third-party libraries are fully acknowledged and respected. Below is a list of said libraries and their respective copyright notices:

App icon: Copyright flaticon.com

Giflib: Copyright (c) 1997 by Eric S. Raymond. All rights reserved.

LCMS2: Copyright (c) 1998-2012 by Marti Maria Saguer. All rights reserved.

Libjpeg-turbo: Copyright (C) 1994-1997 by Thomas G. Lane. Modifications in 2019 by Guido Vollbeding. All rights reserved.

LibWebp: Copyright 2010 by Google Inc. All Rights Reserved.

Tiff: Copyright (c) 1988-1997 by Sam Leffler and Copyright (c) 1991-1997 by Silicon Graphics, Inc. All rights reserved.

The Qt Company Ltd.: Copyright (C) 2016 by The Qt Company Ltd. All rights reserved.

The use of the above-mentioned software within this product is in compliance with the respective licenses and/or terms and conditions of the copyright holders. The inclusion of these third-party libraries does not imply any endorsement by the copyright holders of the products developed using their software.

