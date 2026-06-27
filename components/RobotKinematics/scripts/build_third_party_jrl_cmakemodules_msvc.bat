@echo off
setlocal

if "%QT_CMAKE%"=="" ( echo [ERROR] QT_CMAKE must point to cmake.exe & exit /b 1 )
if "%VCVARS%"=="" ( echo [ERROR] VCVARS must point to vcvars64.bat & exit /b 1 )

set "ROOT=%~dp0.."
set "SRC=%ROOT%\third_party\jrl-cmakemodules"
set "BUILD=%ROOT%\third_party\build\jrl-cmakemodules"
set "INSTALL=%ROOT%\third_party\install\jrl-cmakemodules"

if not exist "%SRC%\CMakeLists.txt" (
    echo [ERROR] jrl-cmakemodules source was not found at %SRC%
    echo         Clone it first: git clone https://github.com/jrl-umi3218/jrl-cmakemodules.git "%SRC%"
    exit /b 1
)

call "%VCVARS%"
if errorlevel 1 exit /b 1

if not exist "%BUILD%" mkdir "%BUILD%"

"%QT_CMAKE%" -S "%SRC%" -B "%BUILD%" -G "Ninja" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX="%INSTALL%"
if errorlevel 1 exit /b 1

"%QT_CMAKE%" --build "%BUILD%"
if errorlevel 1 exit /b 1

"%QT_CMAKE%" --install "%BUILD%"
if errorlevel 1 exit /b 1

echo [OK] jrl-cmakemodules installed to %INSTALL%
exit /b 0
