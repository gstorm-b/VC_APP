@echo off
setlocal

if "%VCVARS%"=="" set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

set "ROOT=%~dp0.."
set "SRC=%ROOT%\third_party\boost"
set "BUILD=%ROOT%\third_party\build\boost"
set "INSTALL=%ROOT%\third_party\install\boost"

if not exist "%SRC%\bootstrap.bat" (
    echo [ERROR] Boost source was not found at %SRC%
    exit /b 1
)

call "%VCVARS%"
if errorlevel 1 exit /b 1

if not exist "%BUILD%" mkdir "%BUILD%"

pushd "%SRC%"

if not exist "b2.exe" (
    call bootstrap.bat
    if errorlevel 1 (
        popd
        exit /b 1
    )
)

b2.exe -j%NUMBER_OF_PROCESSORS% ^
    --build-dir="%BUILD%" ^
    --prefix="%INSTALL%" ^
    toolset=msvc ^
    address-model=64 ^
    architecture=x86 ^
    link=shared ^
    runtime-link=shared ^
    threading=multi ^
    variant=release,debug ^
    --with-filesystem ^
    --with-serialization ^
    --with-system ^
    install
if errorlevel 1 (
    popd
    exit /b 1
)

popd

for %%F in ("%INSTALL%\lib\boost_*.lib") do (
    copy /Y "%%~fF" "%INSTALL%\lib\lib%%~nxF" >nul
)

echo [OK] Boost built and installed to %INSTALL%
exit /b 0
