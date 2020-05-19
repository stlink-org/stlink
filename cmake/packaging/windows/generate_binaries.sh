###
# Build package with binaries for Windows
###

# Install this cross-compiler toolchain:
#sudo apt-get install mingw-w64

# x86_64
mkdir build-mingw
cd build-mingw
cmake -DCMAKE_SYSTEM_NAME=Windows \
      -DTOOLCHAIN_PREFIX=x86_64-w64-mingw32 \
      -DCMAKE_TOOLCHAIN_FILE=./cmake/modules/set_toolchain.cmake ..
make package
cp dist/*.zip ../build/Release/dist
cd ..
rm -rf build-mingw

# i686
mkdir build-mingw
cd build-mingw
cmake -DCMAKE_SYSTEM_NAME=Windows \
      -DTOOLCHAIN_PREFIX=i686-w64-mingw32 \
      -DCMAKE_TOOLCHAIN_FILE=./cmake/modules/set_toolchain.cmake ..
make package
cp dist/*.zip ../build/Release/dist
cd ..
rm -rf build-mingw
