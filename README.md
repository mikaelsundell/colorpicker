# <img src="resources/AppIcon.png" valign="middle" alt="Icon" width="50" height="50"> 3rdparty #

[![License](https://img.shields.io/badge/license-BSD%203--Clause-blue.svg?style=flat-square)](https://github.com/mikaelsundell/icloud-snapshot/blob/master/license.md)

Introduction
------------

<img src="https://github.com/mikaelsundell/colorpicker/blob/4cd961b3f3679f1149b636c70946bb8dadb5f9d1/resources/Colorpicker.png" width="400" />

colorpicker is a mac app to create color palettes.

Documentation
-------------

Colorpicker is a macOS app to easily pick colors across different displays. The app supports display profiles, different color formats, clipboard functionality and an interactive colorwheel for color preview. All internal color processing uses LCMS for platform portability.

**How to use**

Colorpicker needs to be active to pick colors. The app is always active when launched but can be activated using `View > Active [A]`. Once active the application will show the current color interactivly in a hsl color wheel. Use the color picker button or `Edit > Pick [P]` to start. Colors will start to appear on the color wheel. To end the color pick tool use either `Right mouse button` or `Esc` to end. After color pick app will no longer be active, colors can now be checked by clicking on the in the color wheel.

**Display profile**

The default native display values are the native colors from the display. The color values equals a screenshot with the display profile .icc applied. Switch between different display profiles to display color values in different color spaces, transformed from the display profile .icc.

**Color processing in LCMS**

LCMS is a popular color management system for use in open source projects. LCMS closely matches ColorSync, AdobeACE, Reference ICC and other color engines. There may be variations, different engines should roughly match.

Privacy & Security
------------------

colorpicker uses macOS to create images from display and needs permission for `Screen recording`. Add Colorpicker app to the list of allowed applications in `System settings`.

Building
--------

The Colorpicker app can be built both from commandline or using Xcode `-GXcode`. Use `DYLD_IMAGE_SUFFIX=_debug` environment variable to link and run with Qt debug frameworks.

```shell
mkdir build
cd build
cmake .. -DCMAKE_MODULE_PATH=<path>/colorpicker/modules -DCMAKE_PREFIX_PATH=<path>/3rdparty/build/macosx/arm64.debug -GXcode
cmake --build . --config Release -j 8
```

Packaging
---------

The `macdeploy.sh` script will deploy mac bundle to dmg including dependencies.

```shell
./macdeploy.sh -b <path>/Colorpicker.app -m <path>/macdeployqt -d <path>/Colorpicker_macOS12_<arch>.dmg
```

Web Resources
-------------

* GitHub page:        https://github.com/mikaelsundell/colorpicker
* Issues              https://github.com/mikaelsundell/colorpicker/issues

Copyright
---------

* LCMS                https://littlecms.com
* App icon            https://www.flaticon.com/free-icons/color
