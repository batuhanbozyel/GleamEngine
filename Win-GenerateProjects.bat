@echo off
IF EXIST build rmdir /s /q build
IF EXIST bin rmdir /s /q bin
mkdir build
cd build
cmake -G "Visual Studio 17 2022" ..
PAUSE