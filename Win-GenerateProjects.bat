@echo off
rm -rf build
rm -rf bin
mkdir build
cd build
cmake -G "Visual Studio 16 2019" ..
PAUSE