@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
set PATH=C:\Qt\6.8.2\msvc2022_64\bin;%PATH%

if not exist "build_contract_test" mkdir build_contract_test
cd build_contract_test

echo === Running qmake ===
qmake "..\tests\architecture_contract_test\architecture_contract_test.pro" CONFIG+=debug
if errorlevel 1 (
    echo qmake FAILED
    exit /b 1
)

echo === Running nmake ===
nmake
if errorlevel 1 (
    echo nmake FAILED
    exit /b 1
)

echo === BUILD SUCCESS ===
