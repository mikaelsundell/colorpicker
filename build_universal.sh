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
    local merge_path="$2"
    local sign_type="$3"
    local sign_identity="$4"

    case "$sign_type" in
        dylibs)
            find "$bundle_path" -type f ! -path "*.framework/*" | while read -r file; do
                file_type=$(file "$file")
                if [[ "$file_type" == *"Mach-O 64-bit dynamically linked shared library"* ]] || [[ "$file_type" == *"Mach-O 64-bit bundle"* ]]; then
                    merge_file="${file/$bundle_path/$merge_path}"
                    if [ -f "$merge_file" ]; then
                        echo "merging dylib $file with $merge_file version ..."
                        lipo -create "$file" "$merge_file" -output "$file.tmp"
                        mv "$file.tmp" "$file"
                        xattr -c "$file"
                    fi
                    echo "signing dylib $file ..."
                    codesign --force --sign "$sign_identity" --timestamp "$file"
                    echo ""
                fi
            done
            ;;
        frameworks)
            find "$bundle_path" -type d -name "*.framework" | while read -r framework; do
                merge_framework="${framework/$bundle_path/$merge_path}"
                if [ -d "$merge_framework" ]; then
                    echo "merging framework $framework with $merge_framework version ..."
                    for binary in "$framework"/Versions/A/*; do
                        merge_binary="${binary/$framework/$merge_framework}"
                        if [ -f "$merge_binary" ]; then
                            echo "merging $binary with $merge_binary version ..."
                            lipo -create "$binary" "$merge_binary" -output "$binary.tmp"
                            mv "$binary.tmp" "$binary"
                            xattr -c "$file"
                        fi
                    done
                else
                    echo -e "framework $merge_framework does not exist, will not be merged"
                fi
                echo "signing framework $framework ..."
                codesign --force --sign "$sign_identity" --timestamp "$framework"
                echo ""
            done
            ;;
        executables)
            find "$bundle_path" -type f | while read -r file; do
                file_type=$(file "$file")
                if [[ "$file_type" == *"Mach-O 64-bit executable"* ]]; then
                    merge_file="${file/$bundle_path/$merge_path}"
                    if [ -f "$merge_file" ]; then
                        echo "merging $file with $merge_file version ..."
                        lipo -create "$file" "$merge_file" -output "$file.tmp"
                        mv "$file.tmp" "$file"
                        xattr -c "$file"
                    fi
                    echo "signing executable $file with entitlements ..."
                    codesign --force --sign "$sign_identity" --timestamp --options runtime --entitlements "$script_dir/resources/App.entitlements" "$file"
                    echo ""
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
            echo "permissions updated for $file"
            chmod o+r "$file"
        else
            echo "signature verification failed for $file"
        fi
            echo ""
    done
}

# check signing
parse_args() {
    while [[ "$#" -gt 0 ]]; do
        case $1 in
            --target=*) 
                major_version="${1#*=}" ;; 
            --sign)
                sign_code=ON ;;
            *)
                if [[ "$1" == *".app" ]]; then
                    build_apps+=("$1")
                else
                    echo "Unknown option or invalid argument: $1"
                    exit 1
                fi
                ;;
        esac
        shift
    done

    # Check if three .app directories are provided
    if [ "${#build_apps[@]}" -ne 3 ]; then
        echo "Please provide three .app directories."
        exit 1
    fi
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
    local build_app="$1"
    local arm64_app="$2"
    local x86_64_app="$3"

    if [ ! -d "$arm64_app" ] || [ ! -d "$x86_64_app" ]; then
        echo "arm64.app and x86_64.app required to build universal app."
        exit 1
    fi

    pkg_file="$script_dir/Colorpicker_macOS${major_version}_universal.pkg"
    if [ -f "$pkg_file" ]; then
        rm -f "$pkg_file"
    fi

    if [ -d "$build_app" ]; then
        read -p "build_app directory already exists, do you want to remove it before copying? (y/n): " remove_build_app

        if [[ "$remove_build_app" =~ ^[Yy]$ ]]; then
            echo "removing the existing build_app directory ..."
            rm -rf "$build_app"
        else
            echo "please remove or rename the existing build_app directory and try again."
            exit 1
        fi
    fi

    cp -RP "$arm64_app" "$build_app"
    xattr -rc "$build_app"

    if [ -n "$mac_developer_identity" ]; then
        if [ "$sign_code" == "ON" ]; then
            # entitlements
            teamid=$(echo "$mac_developer_identity" | awk -F '[()]' '{print $2}')
            applicationid=$(/usr/libexec/PlistBuddy -c "Print CFBundleIdentifier" "$build_app/Contents/Info.plist")
            entitlements="resources/App.entitlements"
            echo sed -e "s/\${TEAMID}/$teamid/g" -e "s/\${APPLICATIONIDENTIFIER}/$applicationid/g" "$script_dir/resources/App.entitlements.in" > "$entitlements"
            sed -e "s/\${TEAMID}/$teamid/g" -e "s/\${APPLICATIONIDENTIFIER}/$applicationid/g" "$script_dir/resources/App.entitlements.in" > "$entitlements"
            # sign
            sign_app "$build_app" "$x86_64_app" "dylibs" "$mac_developer_identity"
            sign_app "$build_app" "$x86_64_app" "frameworks" "$mac_developer_identity"
            sign_app "$build_app" "$x86_64_app" "executables" "$mac_developer_identity"
            verify_app "$build_app"
            echo codesign --force --deep --sign "$mac_developer_identity" --entitlements $entitlements "$build_app"
            codesign --force --deep --sign "$mac_developer_identity" --entitlements $entitlements "$build_app"
            codesign --verify "$build_app"
        fi
    else 
        echo "Mac Developer identity must be set for appstore distribution, sign will be skipped."
    fi

    # productbuild
    if [ "$sign_code" == "ON" ]; then
        if [ -n "$mac_installer_identity" ]; then
            productbuild --component "$build_app" "/Applications" --sign "${mac_installer_identity}" --product "$build_app/Contents/Info.plist" "$pkg_file" 
        else 
            echo "Mac Installer identity must be set for appstore distribution, sign will be skipped."
            productbuild --component "$build_app" "/Applications" --product "$build_app/Contents/Info.plist" "$pkg_file" 
        fi  
    else
        productbuild --component "$build_app" "/Applications" --product "$build_app/Contents/Info.plist" "$pkg_file" 
    fi
}

build_colorpicker "${build_apps[@]}"