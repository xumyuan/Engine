@echo off
setlocal EnableExtensions DisableDelayedExpansion

:: ============================================================================
:: configure.bat — Configure VS2022 project via CMake preset "default"
:: Usage:
::   configure.bat              — 正常 configure
::   configure.bat --clean      — 重置 CMake 缓存后重新 configure
::   configure.bat --clangd     — 额外生成 clangd 的 compile_commands.json
::   configure.bat --clean --clangd — 重置两个 preset 的缓存后重新生成
:: ============================================================================

set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%"
if errorlevel 1 exit /b 1

set "FRESH_ARG="
set "DO_CLANGD=0"

:: ---------- 解析参数 ----------
:parse_args
if "%~1"=="" goto :args_done
if /i "%~1"=="--clean" (
    set "FRESH_ARG=--fresh"
    shift
    goto :parse_args
)
if /i "%~1"=="--clangd" (
    set "DO_CLANGD=1"
    shift
    goto :parse_args
)
if /i "%~1"=="--help" goto :help
echo [ERROR] Unknown argument: %~1
exit /b 1
:args_done

:: ---------- 检查 cmake ----------
where cmake >nul 2>&1
if errorlevel 1 (
    echo [ERROR] cmake not found in PATH.
    echo         Please install CMake 3.25+ and add it to PATH.
    exit /b 1
)

:: ---------- 检查 VCPKG_ROOT ----------
if not defined VCPKG_ROOT (
    echo [ERROR] Environment variable VCPKG_ROOT is not set.
    echo         Please set it to your vcpkg installation directory.
    exit /b 1
)
if not exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" (
    echo [ERROR] vcpkg.cmake was not found under VCPKG_ROOT.
    exit /b 1
)
:: ---------- clangd 工具链检查 ----------
if "%DO_CLANGD%"=="1" (
    where ninja >nul 2>&1
    if errorlevel 1 (
        echo [ERROR] Ninja is required on PATH for --clangd.
        exit /b 1
    )
    where cl >nul 2>&1
    if errorlevel 1 (
        echo [ERROR] Run --clangd from an x64 Native Tools Command Prompt for VS 2022.
        exit /b 1
    )
    if /i not "%VSCMD_ARG_TGT_ARCH%"=="x64" (
        echo [ERROR] Run --clangd from an x64 Native Tools Command Prompt for VS 2022.
        exit /b 1
    )
)

:: ---------- Configure default (VS2022) ----------
echo.
echo ======================================================
echo  Configuring preset: default (Visual Studio 2022 x64)
echo ======================================================
cmake --preset default %FRESH_ARG%
if errorlevel 1 (
    echo.
    echo [ERROR] CMake configure failed for preset "default".
    exit /b 1
)
echo [OK] preset "default" configured successfully.

:: ---------- Configure clangd（可选） ----------
if "%DO_CLANGD%"=="1" (
    echo.
    echo ======================================================
    echo  Configuring preset: clangd - Ninja/MSVC, compile_commands
    echo ======================================================
    cmake --preset clangd %FRESH_ARG%
    if errorlevel 1 (
        echo.
        echo [ERROR] CMake configure failed for preset "clangd".
        exit /b 1
    ) else (
        echo [OK] clangd database: out\build\clangd\compile_commands.json
    )
)

:: ---------- 完成 ----------
echo.
echo ======================================================
echo  Done! Open out\build\default\Engine.sln in Visual Studio 2022.
echo  Build with: cmake --build --preset debug
echo ======================================================
exit /b 0

:help
echo Usage: configure.bat [--clean] [--clangd] [--help]
echo   --clean   Reset CMake caches for the requested presets using --fresh.
echo   --clangd  Also configure Ninja/MSVC in an x64 VS 2022 tools prompt.
exit /b 0
