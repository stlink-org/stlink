###
# Build package with binaries for Windows
###

# Ensure the target distribution directory exists before copying
mkdir -p build/Release/dist

# i686 (32-bit)
mkdir -p build-mingw-32
cd build-mingw-32
cmake -DTOOLCHAIN_PREFIX=i686-w64-mingw32 \
      -DCMAKE_TOOLCHAIN_FILE=../cmake/modules/set_toolchain.cmake \
      -DCMAKE_SYSTEM_PROCESSOR="i686" \
      -DCMAKE_C_FLAGS="-D_WIN32" \
      -DSTLINK_GENERATE_GUI=OFF \
      ..
make package
sudo cp dist/*.zip ../build/Release/dist
make clean
cd ..


# x86_64 (64-bit)
mkdir -p build-mingw-64
cd build-mingw-64
cmake -DTOOLCHAIN_PREFIX=x86_64-w64-mingw32 \
      -DCMAKE_TOOLCHAIN_FILE=../cmake/modules/set_toolchain.cmake \
      -DCMAKE_SYSTEM_PROCESSOR="x86_64" \
      -DCMAKE_C_FLAGS="-D_WIN32 -D_AMD64_" \
      -DSTLINK_GENERATE_GUI=OFF \
      ..
make package
sudo cp dist/*.zip ../build/Release/dist
make clean
cd ..