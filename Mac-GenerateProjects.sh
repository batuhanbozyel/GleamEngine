#!/bin/sh
rm -rf build
rm -rf bin
mkdir build
cd build
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -G Xcode ..
xattr -w com.apple.xcode.CreatedByBuildSystem true "$PWD/Debug"
xattr -w com.apple.xcode.CreatedByBuildSystem true "$PWD/RelWithDebInfo"
xattr -w com.apple.xcode.CreatedByBuildSystem true "$PWD/Release"
sleep