@echo off
IF EXIST build rmdir /s /q build
IF EXIST bin rmdir /s /q bin
mkdir build
cd build
cmake -G "Visual Studio 18 2026" ..
PAUSE