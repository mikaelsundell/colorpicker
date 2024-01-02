#!/bin/bash
##  macdeploy.sh
##  Colorman
##
##  Copyright (c) 2023 - present Mikael Sundell.
##  All Rights Reserved.

# usage

usage()
{
cat << EOF
macdeploy.sh -- Deploy mac bundle to dmg including depedencies 

usage: $0 [options]

Options:
   -h, --help              Print help message
   -v, --verbose           Print verbose
   -b, --bundle            Path to bundle
   -m, --macdeployqt       Path to macdeployqt
   -d, --dmg               Path to dmg
EOF
}

# parse arguments

i=0; argv=()
for ARG in "$@"; do
    argv[$i]="${ARG}"
    i=$((i + 1))
done

i=0; findex=0
while test $i -lt $# ; do
    ARG="${argv[$i]}"
    case "${ARG}" in
        -h|--help) 
            usage;
            exit;;
        -v|--verbose)
            verbose=1;;
        -b|--bundle) 
            i=$((i + 1)); 
            bundle=${argv[$i]};;
        -d|--dmg) 
            i=$((i + 1)); 
            dmg=${argv[$i]};;
        *) 
            if ! test -e "${ARG}" ; then
                echo "Unknown argument or file '${ARG}'"
            fi;;
    esac
    i=$((i + 1))
done

# test arguments

if [ -z "${bundle}" ] || [ -z "${dmg}" ]; then
    usage
    exit 1
fi

# deploy script
script=$(readlink -f $0)

# deploy dmg
function deploy_dmg() {

    echo "Deploy DMG to ${dmg}"

    dmg_name=`basename ${dmg}`
    dmg_icon="${bundle}/Contents/Resources/AppIcon.icns"
    dmg_temp=`mktemp -q /tmp/${dmg_name}.XXXXXX`
    if ! [ -f "$dmg_temp" ]; then
        echo "Could not create temp directory for dmg, will exit"
        exit 1
    fi

    # hdiutil
    hdiutil create -srcfolder "$bundle" -volname "$dmg_name" -fs HFS+ -fsargs "-c c=64,a=16,e=16" -format UDRW "$dmg_temp"
    dmg_dir="${dmg_temp}.dmg"

    # mount
    dmg_device="$(hdiutil attach -readwrite -noautoopen "$dmg_dir" | awk 'NR==1{print$1}')"
    dmg_volume="$(mount | grep "$dmg_device" | sed 's/^[^ ]* on //;s/ ([^)]*)$//')"

    # icon
    cp "${dmg_icon}" "$dmg_volume/.VolumeIcon.icns"
    SetFile -c icnC "$dmg_volume/.VolumeIcon.icns"
    SetFile -a C "$dmg_volume"

    # dsstore
    # must be created before the dmg is detached and then copied to resources as
    # DS_Store.in and used below to modify the final dmg .DS_Store
    resources_dir=$(dirname ${script})/../resources
    cp "${resources_dir}/DS_Store.in" "$dmg_volume/.DS_Store"

    # applications
    ln -s "/Applications" "$dmg_volume/Applications"

    # github
    cat > "$dmg_volume/Github project.webloc" <<EOL
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>URL</key>
    <string>https://github.com/mikaelsundell/colorpicker</string>
</dict>
</plist>
EOL

    # detach and convert
    hdiutil detach "$dmg_device"
    hdiutil convert "$dmg_dir" -format UDZO -imagekey zlib-level=9 -o "$dmg"
    rm -rf "$dmg_temp" "$dmg_dir"
}

# main
function main {

    if [[ `file "${bundle}" | grep 'directory'` ]]; then
        
        echo "Start to deploy dmg: ${bundle}"
        if [ -f "$dmg" ]; then
            echo "Deploy dmg ${dmg} already exists, will exit"
            exit 1
        fi

        # deploy
        deploy_dmg

    else
        echo "Bundle is not an directory '${bundle}'"
    fi
}

main
