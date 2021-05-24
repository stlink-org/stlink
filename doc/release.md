Release
=======

This document describes the necessary steps for developers to create a release:

1. Update `CHANGELOG.md`, `cmake/packaging/deb/changelog` & `cmake/packaging/rpm/changelog`
2. Update `.version` with semantic version: `x.x.x`
3. Update `README.md` with semantic version `x.x.x` in commits badge
4. Merge `develop` into `master`
5. Create and push git tag and commits `git tag x.x.x`
6. Create binary packages (.rpm / .deb / .zip) with `make package && sh ./cmake/packaging/windows/generate_binaries.sh`
7. Upload packages to the [release page](https://github.com/stlink-org/stlink/releases) of this project
8. Merge `master` into `develop`
9. Update GitHub security policy (/SECURITY.md)
