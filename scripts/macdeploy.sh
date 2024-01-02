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
        -m|--macdeployqt) 
            i=$((i + 1)); 
            macdeployqt=${argv[$i]};;
        *) 
            if ! test -e "${ARG}" ; then
                echo "Unknown argument or file '${ARG}'"
            fi;;
    esac
    i=$((i + 1))
done

# test arguments

if [ -z "${bundle}" ] || [ -z "${macdeployqt}" ]; then
    usage
    exit 1
fi

# deploy script
script=$(readlink -f $0)

# deploy macdeployqt
function deploy_macdeployqt() {

    echo "Deploy Qt to ${bundle}"
    "${macdeployqt}" "${bundle}"
}

# main
function main {

    if [[ `file "${bundle}" | grep 'directory'` ]]; then
        
        echo "Start to deploy bundle: ${bundle}"
        
        # deployqt
        deploy_macdeployqt

    else
        echo "Bundle is not an directory '${bundle}'"
    fi
}

main
