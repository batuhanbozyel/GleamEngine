#!/bin/sh
rm -rf build
rm -rf bin
mkdir build
cd build
cmake -G Xcode ..
xattr -w com.apple.xcode.CreatedByBuildSystem true "$PWD/Debug"
xattr -w com.apple.xcode.CreatedByBuildSystem true "$PWD/RelWithDebInfo"
xattr -w com.apple.xcode.CreatedByBuildSystem true "$PWD/Release"
sleep