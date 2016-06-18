Release
=======

This document describes the steps takes for developers to create a release

1. Update `.version` with semantic version: `x.x.x`
2. Update `README.md` with semantic version `x.x.x` in commits badge
2. Create and push git tag and commits `git tag x.x.x`
3. Create source tarball/zipfile with `make dist`
4. Create binary package with `make package`
