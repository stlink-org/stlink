#!/bin/sh
#
# create the XPM icon and all resolutions below hicolor as PNG

APPNAME="stlink-gui"
ORIGIN="stlink-gui_icon.svg"
OUTDIR="hicolor"

# possible size options are --export-dpi / --export-width / --export-height
OPTS="-z --export-id-only"
ID="scalable-icon"
RESOLUTIONS="16 22 24 32 48 64 128 256"

if ! [ -d $OUTDIR ]; then
    echo "output directory missing. Create it..."
    mkdir $OUTDIR
    for RES in $RESOLUTIONS; do
        mkdir -p $OUTDIR/${RES}x${RES}/apps
    done
fi

# create single app icon
inkscape $OPTS --export-width=32 --export-id=$ID --export-png=$APPNAME.png $ORIGIN
if [ $? != 0 ]; then
    exit 1;
fi
convert $APPNAME.png $APPNAME.xpm

# create all the resolutions
ALL=""
for RES in $RESOLUTIONS; do
    inkscape $OPTS --export-width=$RES --export-id=$ID --export-png=$OUTDIR/${RES}x${RES}/apps/$APPNAME.png $ORIGIN
    ALL="$ALL $OUTDIR/${RES}x${RES}/apps/$APPNAME.png"
done

# this is for windows...
#echo "build Windows icon from $ALL"
#convert $ALL $APPNAME.ico

exit 0
