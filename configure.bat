@echo off

if /I "%~1"=="debug" goto :build_config
if /I "%~1"=="development" goto :build_config
if /I "%~1"=="profile" goto :build_config
if /I "%~1"=="shipping" goto :build_config
goto :run_configure_script

:build_config
set "ENGINE_CONFIG=%~1"
cmake --preset default
if errorlevel 1 exit /b %errorlevel%
cmake --build --preset %ENGINE_CONFIG%
exit /b %errorlevel%

:run_configure_script
setlocal EnableExtensions DisableDelayedExpansion

rem Use Windows PowerShell without loading user profiles. Keep the caller's cwd.
"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\Configure.ps1" %*
exit /b %errorlevel%
