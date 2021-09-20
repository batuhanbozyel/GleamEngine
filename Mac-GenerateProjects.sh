#!/bin/sh
rm -rf build
rm -rf bin
mkdir build
cd build
cmake -G Xcode ..
sleep