@echo off
REM ============================================================================
REM Run the mesh simplification tool on the Nachi MZ04D mesh profile and emit
REM simplified STL meshes plus a paired simplified-profile JSON next to them.
REM
REM The simplified meshes land under presets/Nachi/MZ04/simplified/nachi_mz04d/
REM and the simplified profile is written next to the original Nachi profile
REM so MeshCollisionProfileJsonLoader resolves the relative paths consistently.
REM ============================================================================
setlocal

if "%QT_MSVC_DIR%"=="" set "QT_MSVC_DIR=C:\Qt\6.8.3\msvc2022_64"

set "ROOT=%~dp0.."
set "TOOL_DIR=%ROOT%\build\tools\mesh_simplification"
set "EXE=%TOOL_DIR%\release\mesh_simplification.exe"
if not exist "%EXE%" set "EXE=%TOOL_DIR%\mesh_simplification.exe"

if not exist "%EXE%" (
    echo [ERROR] %EXE% not found. Run scripts\build_mesh_simplification_msvc.bat first.
    exit /b 1
)

set "PATH=%QT_MSVC_DIR%\bin;%PATH%"

set "INPUT=%ROOT%\presets\Nachi\MZ04\nachi_mz04d_mesh_collision.json"
set "OUTPUT=%ROOT%\presets\Nachi\MZ04\nachi_mz04d_mesh_collision_simplified.json"
set "MESH_OUTPUT_DIR=%ROOT%\presets\Nachi\MZ04\simplified\nachi_mz04d"
set "VOXEL_COUNT=20"
set "SAFETY_FACTOR=0.1"

if not "%~1"=="" set "VOXEL_COUNT=%~1"
if not "%~2"=="" set "SAFETY_FACTOR=%~2"

"%EXE%" --input-profile "%INPUT%" --output-profile "%OUTPUT%" --mesh-output-dir "%MESH_OUTPUT_DIR%" --voxel-count %VOXEL_COUNT% --safety-factor %SAFETY_FACTOR% || ( echo [ERROR] mesh simplification run failed & exit /b 1 )

echo [OK] Nachi mesh simplification run completed.
exit /b 0
