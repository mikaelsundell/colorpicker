Readme for colorpicker
======================

[![License](https://img.shields.io/badge/license-BSD%203--Clause-blue.svg?style=flat-square)](https://github.com/mikaelsundell/icloud-snapshot/blob/master/license.md)

Introduction
------------
Colorpicker is a mac app to pick color values on your screen. It's an extended opensource version of macOS Digital Colour Meter app.

Documentation
-------------

The Colorpicker app uses macOS to create images from the display for use with color picking. The app supports display profiles, different color formats and copy to clipboard functionality. All color processesing uses LittleCMS for portability.

**Display profile**

The default native display values are the native colors from the display. The color values equals a screenshot with the display profile .icc applied. Switch between different display profiles to display color values in different color spaces.

**Color processing in LCMS**

LCMS is a popular color management system for use in open source projects. LCMS closely matches ColorSync, AdobeACE, Reference ICC and other color engines. There may be variations although the different engines should closely match.

Privacy & Security
------------------

colorpicker uses macOS to create images from display and needs permission for `Screen recording`. Add Colorpicker app to the list of allowed applications in `System settings`.

Building
--------

The Colourpicker app can be built both from commandline or using Xcode `-GXcode`. Use `DYLD_IMAGE_SUFFIX=_debug` environment variable to link and run with Qt debug frameworks.

```shell
mkdir build
cd build
cmake .. -DCMAKE_MODULE_PATH=<path>/colorpicker/modules -DCMAKE_PREFIX_PATH=<path>/3rdparty/build/macosx/arm64.debug -GXcode
```

Packaging
---------

The `macdeploy.sh` script will deploy mac bundle to dmg including dependencies.

```shell
./macdeploy.sh -b <path>/Colorpicker.app -m <path>/macdeployqt -dmg <path>/Colorpicker_macOS12_<arch>.dmg
```

Web Resources
-------------

* GitHub page:        https://github.com/mikaelsundell/colorpicker
* Issues              https://github.com/mikaelsundell/colorpicker/issues

Copyright
---------

* LittleCMS           https://littlecms.com
* App icon            https://www.flaticon.com/free-icons/color