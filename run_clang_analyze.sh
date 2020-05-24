#!/bin/bash
# Run this hacky script in project root directory to start clang static analysis.
# Adjust ccc-analyzer path if necessary

CCC_ANALYZER=/usr/share/clang/scan-build-3.5/ccc-analyzer
mkdir -p build-clang-analyze/reports
cd build-clang-analyze
cmake -DCMAKE_C_COMPILER=${CCC_ANALYZER} $* ..
scan-build -o ./reports --keep-empty make
