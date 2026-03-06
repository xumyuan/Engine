@echo off
setlocal enabledelayedexpansion

:: ============================================================================
:: configure.bat — Configure VS2022 project via CMake preset "default"
:: Usage:
::   configure.bat              — 正常 configure
::   configure.bat --clean      — 清除 Build 目录后重新 configure
::   configure.bat --clangd     — 额外生成 clangd 的 compile_commands.json
::   configure.bat --clean --clangd — 全部清除后重新生成
:: ============================================================================

set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%"

set "DO_CLEAN=0"
set "DO_CLANGD=0"

:: ---------- 解析参数 ----------
:parse_args
if "%~1"=="" goto :args_done
if /i "%~1"=="--clean" ( set "DO_CLEAN=1" & shift & goto :parse_args )
if /i "%~1"=="--clangd" ( set "DO_CLANGD=1" & shift & goto :parse_args )
echo [WARN] Unknown argument: %~1
shift
goto :parse_args
:args_done

:: ---------- 检查 cmake ----------
where cmake >nul 2>&1
if errorlevel 1 (
    echo [ERROR] cmake not found in PATH.
    echo         Please install CMake 3.25+ and add it to PATH.
    exit /b 1
)

:: 检查 cmake 版本
for /f "tokens=3" %%v in ('cmake --version 2^>^&1 ^| findstr /i "version"') do (
    set "CMAKE_VER=%%v"
)
echo [INFO] Found cmake %CMAKE_VER%

:: ---------- 检查 VCPKG_ROOT ----------
if not defined VCPKG_ROOT (
    echo [ERROR] Environment variable VCPKG_ROOT is not set.
    echo         Please set it to your vcpkg installation directory.
    exit /b 1
)
if not exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" (
    echo [ERROR] VCPKG_ROOT=%VCPKG_ROOT%
    echo         vcpkg.cmake not found. Is vcpkg installed correctly?
    exit /b 1
)
echo [INFO] VCPKG_ROOT=%VCPKG_ROOT%

:: ---------- 清理（可选） ----------
if %DO_CLEAN%==1 (
    if exist "%SCRIPT_DIR%Build" (
        echo [INFO] Removing Build directory...
        rmdir /s /q "%SCRIPT_DIR%Build"
    )
    if %DO_CLANGD%==1 (
        if exist "%SCRIPT_DIR%Build-clangd" (
            echo [INFO] Removing Build-clangd directory...
            rmdir /s /q "%SCRIPT_DIR%Build-clangd"
        )
    )
)

:: ---------- Configure default (VS2022) ----------
echo.
echo ======================================================
echo  Configuring preset: default (Visual Studio 2022 x64)
echo ======================================================
cmake --preset default
if errorlevel 1 (
    echo.
    echo [ERROR] CMake configure failed for preset "default".
    exit /b 1
)
echo [OK] preset "default" configured successfully.

:: ---------- Configure clangd（可选） ----------
if %DO_CLANGD%==1 (
    echo.
    echo ======================================================
    echo  Configuring preset: clangd (Ninja, compile_commands)
    echo ======================================================
    cmake --preset clangd
    if errorlevel 1 (
        echo.
        echo [WARN] CMake configure failed for preset "clangd".
        echo        This is non-critical; VS2022 project is still valid.
    ) else (
        echo [OK] preset "clangd" configured successfully.
    )
)

:: ---------- 完成 ----------
echo.
echo ======================================================
echo  Done! Open Build\Engine.sln in Visual Studio 2022.
echo ======================================================
exit /b 0
