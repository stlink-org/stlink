#!/bin/bash

ISMACOS=$(sw_vers -productVersion)
case $ISMACOS in
10.13*)
    KEXT="stlink_shield_10_13.kext"
    ;;
10.14*)
    KEXT="stlink_shield_10_14.kext"
    ;;
10.15*)
    KEXT="stlink_shield_10_15.kext"
    ;; 
*)
    echo "OS X version not supported."
    exit 1
    ;;
esac
chown -R root:wheel $KEXT/
cp -R $KEXT /System/Library/Extensions/stlink_shield.kext
kextload -v /System/Library/Extensions/stlink_shield.kext
touch /System/Library/Extensions
