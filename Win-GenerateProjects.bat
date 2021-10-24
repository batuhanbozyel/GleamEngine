@echo off
IF EXIST build rmdir /s /q build
IF EXIST rmdir /s /q bin
mkdir build
cd build
cmake -G "Visual Studio 16 2019" ..
PAUSE