#!/bin/bash
##  Copyright 2022-present Contributors to the 3rdparty project.
##  SPDX-License-Identifier: BSD-3-Clause
##  https://github.com/mikaelsundell/3rdparty

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
machine_arch=$(uname -m)
macos_version=$(sw_vers -productVersion)
major_version=$(echo "$macos_version" | cut -d '.' -f 1)

# signing
sign_code=OFF
code_sign_identity=""
development_team_id=""

# Create distribution.xml for the pkg file
create_distribution_xml() {
    local identifier="$1"
    local version="$2"
    local pkg_file="$3"

    cat <<EOF > distribution.xml
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="1">
    <title>Colorpicker</title>
    <options customize="never" require-scripts="false"/>
    <domains enable_anywhere="false" enable_currentUserHome="false" enable_localSystem="true"/>
    <volume-check>
        <allowed-os-versions>
            <os-version min="10.12"/>
        </allowed-os-versions>
    </volume-check>
    <installation-check script="installerCheckScript()"/>
    <script>
        <![CDATA[
        function installerCheckScript() {
            if (system.version.ProductVersion < '10.12') {
                my.result.title = 'Unsupported macOS version';
                my.result.message = 'This package requires macOS 10.12 or later.';
                my.result.type = 'Fatal';
                return false;
            }
            return true;
        }
        ]]>
    </script>
    <choices-outline>
        <line choice="default">
            <line choice="application"/>
        </line>
    </choices-outline>
    <choice id="default" title="Colorpicker">
        <pkg-ref id="$identifier"/>
    </choice>
    <choice id="application" visible="false">
        <pkg-ref id="$identifier"/>
    </choice>
    <pkg-ref id="$identifier" version="$version" onConclusion="none">$pkg_file</pkg-ref>
</installer-gui-script>
EOF
}

# check signing
parse_args() {
    while [[ "$#" -gt 0 ]]; do
        case $1 in
            --target=*) 
                major_version="${1#*=}" ;;
            --sign)
                sign_code=ON ;;
            --deploy)
                deploy=ON ;;
            --pkg)
                pkg=ON ;;
            *)
                build_type="$1" # save it in build_type if it's not a recognized flag
                ;;
        esac
        shift
    done
}
parse_args "$@"

# target
if [ -z "$major_version" ]; then
    macos_version=$(sw_vers -productVersion)
    major_version=$(echo "$macos_version" | cut -d '.' -f 1)
fi
export MACOSX_DEPLOYMENT_TARGET=$major_version
export CMAKE_OSX_DEPLOYMENT_TARGET=$major_version

# signing
if [ "$sign_code" == "ON" ]; then
    default_code_sign_identity=${CODE_SIGN_IDENTITY:-}
    default_development_team_id=${DEVELOPMENT_TEAM_ID:-}

    read -p "Enter Code Sign Identity [$default_code_sign_identity]: " input_code_sign_identity
    code_sign_identity=${input_code_sign_identity:-$default_code_sign_identity}

    if [[ ! "$code_sign_identity" == *"Developer ID"* ]] && [[ ! "$code_sign_identity" == *"3rd Party Mac Developer"* ]]; then
        echo "Code Sign identity must contain 'Developer ID' or '3rd Party Mac Developer' to be a valid certificate."
        exit 1
    fi

    read -p "Enter Development Team ID [$default_development_team_id]: " input_development_team_id
    development_team_id=${input_development_team_id:-$default_development_team_id}
fi

# exit on error
set -e 

# clear
clear

# build type
if [ "$build_type" != "debug" ] && [ "$build_type" != "release" ] && [ "$build_type" != "all" ]; then
    echo "invalid build type: $build_type (use 'debug', 'release', or 'all')"
    exit 1
fi

echo "Building Colorpicker for $build_type"
echo "---------------------------------"

# check if cmake is in the path
if ! command -v cmake &> /dev/null; then
    echo "cmake not found in the PATH, will try to set to /Applications/CMake.app/Contents/bin"
    export PATH=$PATH:/Applications/CMake.app/Contents/bin
    if ! command -v cmake &> /dev/null; then
        echo "cmake could not be found, please make sure it's installed"
        exit 1
    fi
fi

# check if cmake version is compatible
if ! [[ $(cmake --version | grep -o '[0-9]\+\(\.[0-9]\+\)*' | head -n1) < "3.28.0" ]]; then
    echo "cmake version is not compatible with Qt, must be before 3.28.0 for multi configuration"
    exit 1;
fi

# build colorpicker
build_colorpicker() {
    local build_type="$1"

    # cmake
    export PATH=$PATH:/Applications/CMake.app/Contents/bin &&

    # script dir
    cd "$script_dir"

    # clean dir
    build_dir="$script_dir/build.$build_type"
    if [ -d "$build_dir" ]; then
        rm -rf "$build_dir"
    fi

    # build dir
    mkdir -p "build.$build_type"
    cd "build.$build_type"

    # prefix dir
    if ! [ -d "$THIRDPARTY_DIR" ]; then
        echo "could not find 3rdparty project in: $THIRDPARTY_DIR"
        exit 1
    fi
    prefix="$THIRDPARTY_DIR"

    # xcode build
    xcode_type=$(echo "$build_type" | awk '{ print toupper(substr($0, 1, 1)) tolower(substr($0, 2)) }')

    # build
    cmake .. -DCMAKE_MODULE_PATH="$script_dir/modules" -DCMAKE_PREFIX_PATH="$prefix" -G Xcode && 
    cmake --build . --config $xcode_type --parallel &&
    if [ "$deploy" == "ON" ]; then
        dmg_file="$script_dir/Colorpicker_macOS${major_version}_${machine_arch}_${build_type}.dmg"
        if [ -f "$dmg_file" ]; then
            rm -f "$dmg_file"
        fi

        # deploy
        $script_dir/scripts/macdeploy.sh -b "$xcode_type/Colorpicker.app" -m "$prefix/bin/macdeployqt"
        if [ "$sign_code" == "ON" ]; then
            codesign --force --deep --sign "$code_sign_identity" --timestamp --options runtime "$xcode_type/Colorpicker.app"
        fi

        # deploydmg
        $script_dir/scripts/macdmg.sh -b "$xcode_type/Colorpicker.app" -d "$dmg_file"
        if [ "$sign_code" == "ON" ]; then
            codesign --force --deep --sign "$code_sign_identity" --timestamp --options runtime --verbose "$dmg_file"
        fi
    fi

    if [ "$pkg" == "ON" ]; then
        pkg_file="$script_dir/Colorpicker_macOS${major_version}_${machine_arch}_${build_type}.pkg"
        if [ -f "$pkg_file" ]; then
            rm -f "$pkg_file"
        fi

        # deploy
        $script_dir/scripts/macdeploy.sh -b "$xcode_type/Colorpicker.app" -m "$prefix/bin/macdeployqt"
        
        # plist
        plist_path="$xcode_type/Colorpicker.app/Contents/Info.plist"
        if [ ! -f "$plist_path" ]; then
            echo "pkg version and identifier could not be found, info.plist not found in $xcode_type/Colorpicker.app"
            exit 1
        fi
        version=$(/usr/libexec/PlistBuddy -c "Print CFBundleVersion" "$plist_path")
        identifier=$(/usr/libexec/PlistBuddy -c "Print CFBundleIdentifier" "$plist_path")

        # create pkg
        pkgbuild --component "$xcode_type/Colorpicker.app" --identifier "$identifier" --version "$version" --install-location "/Applications" "intermediate.pkg"

        # Create distribution.xml
        create_distribution_xml "$identifier" "$version" "intermediate.pkg"

        # Use productbuild to create the final pkg
        productbuild --distribution distribution.xml --package-path . "$pkg_file"

        if [ "$sign_code" == "ON" ]; then
            productsign --sign "$code_sign_identity" "$pkg_file" "$pkg_file.signed"
            mv "$pkg_file.signed" "$pkg_file"
        fi
    fi
}

# build types
if [ "$build_type" == "all" ]; then
    build_colorpicker "debug"
    build_colorpicker "release"
else
    build_colorpicker "$build_type"
fi
