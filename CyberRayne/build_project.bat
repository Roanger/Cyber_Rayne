@echo off
rem Build script for Cyber Rayne
rem Set up Visual Studio environment
call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat" -arch=x64

rem Create and configure the build directory
if not exist build (
    mkdir build
)
cd build
cmake .. -G "Visual Studio 17 2022" -A x64

rem Build the project with CMake
cmake --build . --config Debug

rem Run the executable
cd Debug
CyberRayne.exe
cd ../..
pause

pause
