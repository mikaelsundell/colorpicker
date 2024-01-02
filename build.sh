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

# check signing
parse_args() {
    while [[ "$#" -gt 0 ]]; do
        case $1 in
            --target=*) 
                major_version="${1#*=}" ;;
            --sign)
                sign_code=ON ;;
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

    if [[ ! "$code_sign_identity" == *"Developer ID"* ]]; then
        echo "Code Sign identity must contain 'Developer ID' to be a valid developer certificate."
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
build_type="$1"
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
    prefix="$script_dir/../3rdparty/build/macosx/$machine_arch.$build_type"
    if ! [ -d "$prefix" ]; then
        echo "could not find 3rdparty for build in: $prefix"
        exit 1;
    fi

    # xcode build
    xcode_type=$(echo "$build_type" | awk '{ print toupper(substr($0, 1, 1)) tolower(substr($0, 2)) }')

    # build
    cmake .. -DCMAKE_MODULE_PATH="$script_dir/modules" -DCMAKE_PREFIX_PATH="$prefix" -G Xcode &&
    cmake --build . --config $xcode_type --parallel

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
}

# build types
if [ "$build_type" == "all" ]; then
    build_colorpicker "debug"
    build_colorpicker "release"
else
    build_colorpicker "$build_type"
fi
