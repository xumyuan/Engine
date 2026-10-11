@echo off
setlocal EnableExtensions DisableDelayedExpansion

rem Use Windows PowerShell without loading user profiles. Keep the caller's cwd.
"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\Configure.ps1" %*
exit /b %errorlevel%
