Release
=======

This document describes the necessary steps for developers to create a release:

1. Update `CHANGELOG.md`
2. Update `.version` with semantic version: `x.x.x`
3. Update `README.md` with semantic version `x.x.x` in commits badge
4. Create and push git tag and commits `git tag x.x.x`
5. Create binary packages (.rpm / .deb / .zip) with `make package && sh ./cmake/packaging/windows/generate_binaries.sh`
6. Upload packages to the [release page](https://github.com/stlink-org/stlink/releases) of this project
