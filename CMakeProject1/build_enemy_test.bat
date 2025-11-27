@echo off
REM Simple build script for EnemyTypesTest

REM Check if we're in the right directory
echo Building EnemyTypesTest...

REM Compile the test directly with g++ if available
g++ -std=c++17 -Iinclude src/entities/Enemy.cpp src/entities/EnemyTypes.cpp tests/EnemyTypesTest.cpp -o EnemyTypesTest.exe

if %errorlevel% == 0 (
    echo Build successful!
    echo Running EnemyTypesTest...
    .\EnemyTypesTest.exe
) else (
    echo Build failed. Trying with clang++...
    clang++ -std=c++17 -Iinclude src/entities/Enemy.cpp src/entities/EnemyTypes.cpp tests/EnemyTypesTest.cpp -o EnemyTypesTest.exe
    
    if %errorlevel% == 0 (
        echo Build successful!
        echo Running EnemyTypesTest...
        .\EnemyTypesTest.exe
    ) else (
        echo Build failed with both g++ and clang++.
        echo Please ensure you have a C++ compiler installed.
    )
)

echo.
pause
