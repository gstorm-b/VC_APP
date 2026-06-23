@echo off
setlocal

if "%QT_CMAKE%"=="" set "QT_CMAKE=C:\Qt\Tools\CMake_64\bin\cmake.exe"
if "%VCVARS%"=="" set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

set "ROOT=%~dp0.."
set "SRC=%ROOT%\third_party\fcl"
set "BUILD=%ROOT%\third_party\build\fcl"
set "INSTALL=%ROOT%\third_party\install\fcl"
set "LIBCCD_INSTALL=%ROOT%\third_party\install\libccd"
set "EIGEN_INCLUDE=%ROOT%\third_party\eigen"

if not exist "%SRC%\CMakeLists.txt" (
    echo [ERROR] FCL source was not found at %SRC%
    exit /b 1
)

if not exist "%LIBCCD_INSTALL%\lib\ccd\ccd-config.cmake" (
    echo [ERROR] libccd install was not found at %LIBCCD_INSTALL%
    echo         Run scripts\build_third_party_libccd_msvc.bat first.
    exit /b 1
)

call "%VCVARS%"
if errorlevel 1 exit /b 1

if not exist "%BUILD%" mkdir "%BUILD%"

"%QT_CMAKE%" -S "%SRC%" -B "%BUILD%" -G "Ninja" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX="%INSTALL%" ^
    -DCMAKE_PREFIX_PATH="%LIBCCD_INSTALL%" ^
    -DEIGEN3_INCLUDE_DIR="%EIGEN_INCLUDE%" ^
    -DFCL_WITH_OCTOMAP=OFF ^
    -DBUILD_TESTING=OFF ^
    -DFCL_STATIC_LIBRARY=ON
if errorlevel 1 exit /b 1

"%QT_CMAKE%" --build "%BUILD%"
if errorlevel 1 exit /b 1

"%QT_CMAKE%" --install "%BUILD%"
if errorlevel 1 exit /b 1

echo [OK] FCL built and installed to %INSTALL%
exit /b 0
