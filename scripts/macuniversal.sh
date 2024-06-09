#!/bin/bash
##  Copyright 2022-present Contributors to the colorpicker project.
##  SPDX-License-Identifier: BSD-3-Clause
##  https://github.com/mikaelsundell/colorpicker

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
machine_arch=$(uname -m)
macos_version=$(sw_vers -productVersion)
major_version=$(echo "$macos_version" | cut -d '.' -f 1)

# signing
sign_code=OFF
mac_developer_identity=""
mac_installer_identity=""

sign_app() {
    local bundle_path="$1"
    local sign_type="$2"
    local sign_identity="$3"

    case "$sign_type" in
        dylibs)
            find "$bundle_path" -type f ! -path "*.framework/*" | while read -r file; do
                file_type=$(file "$file")
                if [[ "$file_type" == *"Mach-O 64-bit dynamically linked shared library"* ]] || [[ "$file_type" == *"Mach-O universal binary"* ]]; then
                    echo "signing dylib $file..."
                    codesign --force --sign "$sign_identity" --timestamp "$file"
                fi
            done
            ;;
        frameworks)
            find "$bundle_path" -type d -name "*.framework" | while read -r framework; do
                echo "signing framework $framework..."
                codesign --force --sign "$sign_identity" --timestamp "$framework"
            done
            ;;
        executables)
            find "$bundle_path" -type f | while read -r file; do
                file_type=$(file "$file")
                if [[ "$file_type" == *"Mach-O 64-bit executable"* ]]; then
                    echo "signing executable $file with entitlements ..."
                    codesign --force --sign "$sign_identity" --timestamp --options runtime --entitlements "App.entitlements" "$file"
                fi
            done
            ;;
        *)
            echo "unknown sign type: $sign_type"
            exit 1
            ;;
    esac
}

verify_app() {
    local bundle_path="$1"
    find "$bundle_path" -type f \( -name "*.dylib" -o -name "*.so" -o -name "*.bundle" -o -name "*.framework" -o -perm +111 \) | while read -r file; do
        echo "verifying $file..."
        if codesign --verify --verbose "$file"; then
            echo "signature verification passed for $file"
        else
            echo "signature verification failed for $file"
        fi
    done
}

# check signing
parse_args() {
    while [[ "$#" -gt 0 ]]; do
        case $1 in
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

# exit on error
set -e 

# clear
clear

echo "Building Colorpicker for universal"
echo "---------------------------------"

# signing
if [ "$sign_code" == "ON" ]; then
    default_developerid_identity=${DEVELOPERID_IDENTITY:-}
    default_mac_developer_identity=${MAC_DEVELOPER_IDENTITY:-}
    default_mac_installer_identity=${MAC_INSTALLER_IDENTITY:-}

    read -p "enter Developer ID certificate identity [$default_developerid_identity]: " input_developerid_identity
    developerid_identity=${input_developerid_identity:-$default_developerid_identity}

    if [[ ! "$developerid_identity" == *"Developer ID"* ]]; then
        echo "Developer ID certificate identity must contain 'Developer ID', required for github distribution."
    fi

    read -p "enter Mac Developer certificate Identity [$default_mac_developer_identity]: " input_mac_developer_identity
    mac_developer_identity=${input_mac_developer_identity:-$default_mac_developer_identity}

    if [[ ! "$mac_developer_identity" == *"3rd Party Mac Developer Application"* ]]; then
        echo "Mac Developer installer identity must contain '3rd Party Mac Developer Installer', required for appstore distribution."
    fi

    read -p "enter Mac Installer certificate Identity [$default_mac_installer_identity]: " input_mac_installer_identity
    mac_installer_identity=${input_mac_installer_identity:-$default_mac_installer_identity}

    if [[ ! "$mac_installer_identity" == *"3rd Party Mac Developer Installer"* ]]; then
        echo "Mac Developer installer identity must contain '3rd Party Mac Developer Installer', required for appstore distribution."
    fi
    echo ""
fi

# build colorpicker
build_colorpicker() {
    local build_type="$1"

    # cmake
    export PATH=$PATH:/Applications/CMake.app/Contents/bin &&

    # script dir
    cd "$script_dir"

        pkg_file="$script_dir/Colorpicker_macOS${major_version}_${machine_arch}_${build_type}.pkg"
        if [ -f "$pkg_file" ]; then
            rm -f "$pkg_file"
        fi

        # provisioning
        if [ -n "$provisioning_profile" ] && [ -n "$provisioning_profile_path" ]; then
            cp -f "$provisioning_profile_path" "$xcode_type/Colorpicker.app/Contents/embedded.provisionprofile"
        else
            echo "Provisioning profile and path must be set for appstore distribution, will be skipped."
        fi

        # deploy
        $script_dir/scripts/macdeploy.sh -b "$xcode_type/Colorpicker.app" -m "$prefix/bin/macdeployqt"
        if [ -n "$mac_developer_identity" ]; then
            if [ "$sign_code" == "ON" ]; then
                # entitlements
                teamid=$(echo "$mac_developer_identity" | awk -F '[()]' '{print $2}')
                applicationid=$(/usr/libexec/PlistBuddy -c "Print CFBundleIdentifier" "$xcode_type/Colorpicker.app/Contents/Info.plist")
                entitlements="App.entitlements"
                echo sed -e "s/\${TEAMID}/$teamid/g" -e "s/\${APPLICATIONIDENTIFIER}/$applicationid/g" "$script_dir/resources/App.entitlements.in" > "$entitlements"
                sed -e "s/\${TEAMID}/$teamid/g" -e "s/\${APPLICATIONIDENTIFIER}/$applicationid/g" "$script_dir/resources/App.entitlements.in" > "$entitlements"
                # sign
                sign_app "$xcode_type/Colorpicker.app" "dylibs" "$mac_developer_identity"
                sign_app "$xcode_type/Colorpicker.app" "frameworks" "$mac_developer_identity"
                sign_app "$xcode_type/Colorpicker.app" "executables" "$mac_developer_identity"
                verify_app "$xcode_type/Colorpicker.app"
                codesign --force --deep --sign "$mac_developer_identity" --entitlements $entitlements "$xcode_type/Colorpicker.app"
                codesign --verify "$xcode_type/Colorpicker.app"
            fi
        else 
            echo "Mac Developer identity must be set for appstore distribution, sign will be skipped."
        fi

        # productbuild
        if [ "$sign_code" == "ON" ]; then
            if [ -n "$mac_installer_identity" ]; then
                productbuild --component "$xcode_type/Colorpicker.app" "/Applications" --sign "${mac_installer_identity}" --product "$xcode_type/Colorpicker.app/Contents/Info.plist" "$pkg_file" 
            else 
                echo "Mac Installer identity must be set for appstore distribution, sign will be skipped."
                productbuild --component "$xcode_type/Colorpicker.app" "/Applications" --product "$xcode_type/Colorpicker.app/Contents/Info.plist" "$pkg_file" 
            fi  
        else
            productbuild --component "$xcode_type/Colorpicker.app" "/Applications" --product "$xcode_type/Colorpicker.app/Contents/Info.plist" "$pkg_file" 
        fi
}

