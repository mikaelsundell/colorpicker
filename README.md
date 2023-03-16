Readme for colorpicker
======================

[![License](https://img.shields.io/badge/license-BSD%203--Clause-blue.svg?style=flat-square)](https://github.com/mikaelsundell/icloud-snapshot/blob/master/license.md)

Introduction
------------

![Screenshot](https://github.com/mikaelsundell/colorpicker/blob/a408dc66fed59e93153df4822b4336cd33055706/resources/Colorpicker.png)

Colorpicker is a mac app to pick color values on your screen. It's an extended opensource version of macOS Digital Colour Meter app.

Documentation
-------------

The Colorpicker app uses macOS to create images from the display for use with color picking. The app supports display profiles, different color formats and copy to clipboard functionality. All display color processing uses LCMS for platform portability.

**Display profile**

The default native display values are the native colors from the display. The color values equals a screenshot with the display profile .icc applied. Switch between different display profiles to display color values in different color spaces, transformed from the display profile .icc.

**Picking colors**

Use Pick icon or `Edit > Pick [P]` to pick colors from any screen and window and each color is stored in the color wheel, use right mouse button or escape to exit picker. Use `Aperture`, `Marker size`, `Background opacity` and `Color angle`to adjust the color wheel to your preference. To go back and check picked colors use `View > Freeze [F]` and color will be clickable in the color wheel.

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
