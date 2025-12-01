@echo off
:: Build test for Character Selection System

:: Compile the test
cl /EHsc /I include /I third_party src/tests/CharacterSelectionTest.cpp src/systems/CharacterSelectionSystem.cpp src/entities/Player.cpp src/entities/Item.cpp src/entities/Spell.cpp /link /OUT:CharacterSelectionTest.exe

:: Run the test if compilation succeeded
if %ERRORLEVEL% EQU 0 (
    echo Compilation successful!
    CharacterSelectionTest.exe
) else (
    echo Compilation failed.
)
