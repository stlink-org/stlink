# make ... for stlink v1 MACOSx driver
##

# Package Maker Binary
PMBIN=/Developer/Applications/Utilities/PackageMaker.app/Contents/MacOS/PackageMaker

all:
	xcodebuild -project stlink_shield/stlink_shield.xcodeproj -alltargets
	$(PMBIN) --doc stlink_shield.pmdoc --no-recommend --verbose

clean:
	rm -rf stlink_shield/build
	rm -rf /tmp/stlink*
	# rm -rf stlinkshield.mpkg
