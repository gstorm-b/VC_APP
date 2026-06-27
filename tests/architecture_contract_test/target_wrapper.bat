@echo off
SetLocal EnableDelayedExpansion
if "%QT_MSVC_DIR%"=="" (
    echo [ERROR] QT_MSVC_DIR must point to the Qt MSVC kit root.
    exit /b 1
)
(set PATH=%QT_MSVC_DIR%\bin;!PATH!)
if defined QT_PLUGIN_PATH (
    set QT_PLUGIN_PATH=%QT_MSVC_DIR%\plugins;!QT_PLUGIN_PATH!
) else (
    set QT_PLUGIN_PATH=%QT_MSVC_DIR%\plugins
)
%*
EndLocal
