@echo off
setlocal

if "%QT_CMAKE%"=="" ( echo [ERROR] QT_CMAKE must point to cmake.exe & exit /b 1 )
if "%VCVARS%"=="" ( echo [ERROR] VCVARS must point to vcvars64.bat & exit /b 1 )

set "ROOT=%~dp0.."
set "SRC=%ROOT%\third_party\coal"
set "BUILD=%ROOT%\third_party\build\coal"
set "INSTALL=%ROOT%\third_party\install\coal"
set "FCL_INSTALL=%ROOT%\third_party\install\fcl"
set "LIBCCD_INSTALL=%ROOT%\third_party\install\libccd"
set "ASSIMP_INSTALL=%ROOT%\third_party\install\assimp"
set "BOOST_INSTALL=%ROOT%\third_party\install\boost"
set "EIGEN_INCLUDE=%ROOT%\third_party\eigen"
set "JRL_INSTALL=%ROOT%\third_party\install\jrl-cmakemodules"

if not exist "%SRC%\CMakeLists.txt" (
    echo [ERROR] Coal source was not found at %SRC%
    exit /b 1
)

if not exist "%JRL_INSTALL%\share\cmake\jrl-cmakemodules\jrl-cmakemodulesConfig.cmake" (
    echo [ERROR] jrl-cmakemodules install was not found at %JRL_INSTALL%
    echo         Run scripts\build_third_party_jrl_cmakemodules_msvc.bat first.
    exit /b 1
)

call "%VCVARS%"
if errorlevel 1 exit /b 1

set "LIB=%BOOST_INSTALL%\lib;%LIB%"
set "PATH=%ASSIMP_INSTALL%\bin;%BOOST_INSTALL%\lib;%PATH%"

if not exist "%BUILD%" mkdir "%BUILD%"

"%QT_CMAKE%" -S "%SRC%" -B "%BUILD%" -G "Ninja" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX="%INSTALL%" ^
    -DCMAKE_PREFIX_PATH="%JRL_INSTALL%;%FCL_INSTALL%;%LIBCCD_INSTALL%;%ASSIMP_INSTALL%" ^
    -DEigen3_DIR="" ^
    -DEIGEN3_INCLUDE_DIR="%EIGEN_INCLUDE%" ^
    -DASSIMP_ROOT_DIR="%ASSIMP_INSTALL%" ^
    -DBoost_ROOT="%BOOST_INSTALL%" ^
    -DBOOST_ROOT="%BOOST_INSTALL%" ^
    -DBoost_INCLUDE_DIR="%BOOST_INSTALL%\include\boost-1_87" ^
    -DBoost_LIBRARY_DIR_RELEASE="%BOOST_INSTALL%\lib" ^
    -DBoost_LIBRARY_DIR_DEBUG="%BOOST_INSTALL%\lib" ^
    -DBoost_NO_SYSTEM_PATHS=ON ^
    -DBUILD_TESTING=OFF ^
    -DBUILD_PYTHON_INTERFACE=OFF ^
    -DINSTALL_DOCUMENTATION=OFF ^
    -DCOAL_ENABLE_LOGGING=OFF
if errorlevel 1 exit /b 1

"%QT_CMAKE%" --build "%BUILD%"
if errorlevel 1 exit /b 1

"%QT_CMAKE%" --install "%BUILD%"
if errorlevel 1 exit /b 1

echo [OK] Coal built and installed to %INSTALL%
exit /b 0
