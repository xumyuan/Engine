# Compatible with Windows PowerShell 5.1. No modules or user profiles required.
Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

function Resolve-ConfigurePath {
    param([string]$Root, [string]$Path)
    if ([IO.Path]::IsPathRooted($Path)) { return [IO.Path]::GetFullPath($Path) }
    return [IO.Path]::GetFullPath((Join-Path $Root $Path))
}

function Get-ConfigureFingerprint {
    param([string]$Root, [string[]]$Inputs, [string[]]$Outputs, [string]$CMake)

    $files = @($Inputs) + @($Outputs) + @(
        "$Root/configure.bat", "$Root/scripts/Configure.ps1",
        "$Root/vcpkg.json", "$Root/vcpkg-configuration.json"
    )
    # Preset includes are not part of CMake's file API inputs.
    $pending = @("$Root/CMakePresets.json", "$Root/CMakeUserPresets.json")
    $visited = @{}
    while ($pending.Count -gt 0) {
        $path = [IO.Path]::GetFullPath($pending[0])
        $pending = @($pending | Select-Object -Skip 1)
        if ($visited.ContainsKey($path)) { continue }
        $visited[$path] = $true
        $files += $path
        if (!(Test-Path -LiteralPath $path -PathType Leaf)) { continue }
        $preset = Get-Content -LiteralPath $path -Raw -Encoding UTF8 | ConvertFrom-Json
        if ($preset.PSObject.Properties['include']) {
            foreach ($include in $preset.include) {
                if ($include.Contains('$')) { throw 'Preset include macros require a full configure.' }
                $pending += Resolve-ConfigurePath (Split-Path $path -Parent) $include
            }
        }
    }
    $overlayDirs = @("$Root/ports")
    $configPath = "$Root/vcpkg-configuration.json"
    if (Test-Path -LiteralPath $configPath) {
        $config = Get-Content -LiteralPath $configPath -Raw -Encoding UTF8 | ConvertFrom-Json
        foreach ($key in @('overlay-ports', 'overlay-triplets')) {
            if ($config.PSObject.Properties[$key]) {
                foreach ($dir in $config.$key) { $overlayDirs += Resolve-ConfigurePath $Root $dir }
            }
        }
    }
    foreach ($key in @('VCPKG_OVERLAY_PORTS', 'VCPKG_OVERLAY_TRIPLETS')) {
        $value = [Environment]::GetEnvironmentVariable($key)
        if ($value) {
            foreach ($dir in $value.Split(';')) {
                if ($dir) { $overlayDirs += Resolve-ConfigurePath $Root $dir }
            }
        }
    }
    $records = New-Object 'System.Collections.Generic.List[string]'
    foreach ($dir in ($overlayDirs | Sort-Object -Unique)) {
        $records.Add("overlay:$dir")
        if (Test-Path -LiteralPath $dir -PathType Container) {
            $files += @(Get-ChildItem -LiteralPath $dir -Recurse -File | ForEach-Object FullName)
        } else { $records.Add("missing:$dir") }
    }
    # Source contents affect compilation, not configuration. Track glob membership only.
    if (Test-Path -LiteralPath "$Root/Source") {
        Get-ChildItem -LiteralPath "$Root/Source" -Recurse -File |
            Where-Object { $_.Extension -in @('.cpp', '.h') } |
            Sort-Object FullName | ForEach-Object { $records.Add("source:$($_.FullName)") }
    }
    if (Test-Path -LiteralPath "$Root/Source/shaders") {
        Get-ChildItem -LiteralPath "$Root/Source/shaders" -Recurse -File -Filter '*.glsl' |
            Sort-Object FullName | ForEach-Object { $records.Add("shader:$($_.FullName)") }
    }
    foreach ($path in ($files | Sort-Object -Unique)) {
        if (Test-Path -LiteralPath $path -PathType Leaf) {
            $records.Add("file:$path`:$((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash)")
        } else { $records.Add("missing:$path") }
    }
    foreach ($name in @(
        'PATH', 'VCPKG_ROOT', 'VCPKG_DEFAULT_TRIPLET', 'VCPKG_DEFAULT_HOST_TRIPLET',
        'VCPKG_OVERLAY_PORTS', 'VCPKG_OVERLAY_TRIPLETS', 'VCPKG_FEATURE_FLAGS',
        'VCPKG_BINARY_SOURCES', 'VCPKG_CHAINLOAD_TOOLCHAIN_FILE', 'VCPKG_VISUAL_STUDIO_PATH',
        'CC', 'CXX', 'CFLAGS', 'CXXFLAGS', 'LDFLAGS', 'INCLUDE', 'LIB', 'LIBPATH',
        'VSINSTALLDIR', 'VCINSTALLDIR', 'VCToolsInstallDir', 'WindowsSdkDir',
        'WindowsSDKVersion', 'VSCMD_ARG_TGT_ARCH', 'CMAKE_PREFIX_PATH',
        'CMAKE_GENERATOR', 'CMAKE_TOOLCHAIN_FILE'
    )) {
        $records.Add("env:$name=$([Environment]::GetEnvironmentVariable($name))")
    }
    foreach ($path in @($CMake, "$env:VCPKG_ROOT/vcpkg.exe")) {
        if (Test-Path -LiteralPath $path -PathType Leaf) {
            $item = Get-Item -LiteralPath $path
            $records.Add("tool:$path`:$($item.Length):$($item.LastWriteTimeUtc.Ticks)")
        } else { $records.Add("missing-tool:$path") }
    }
    $sha = [Security.Cryptography.SHA256]::Create()
    try {
        $bytes = [Text.Encoding]::UTF8.GetBytes(($records -join "`n"))
        return [BitConverter]::ToString($sha.ComputeHash($bytes)).Replace('-', '')
    } finally { $sha.Dispose() }
}

function Get-ConfigureInputs {
    param([string]$Root, [string]$BuildDir, [string]$PreviousIndex)

    $replyDir = "$BuildDir/.cmake/api/v1/reply"
    $indexFile = Get-ChildItem -LiteralPath $replyDir -Filter 'index-*.json' |
        Sort-Object Name -Descending | Select-Object -First 1
    if (!$indexFile) { throw 'CMake did not provide a file API reply.' }
    if ($indexFile.FullName -eq $PreviousIndex) { throw 'CMake did not refresh the file API reply.' }
    $index = Get-Content -LiteralPath $indexFile.FullName -Raw -Encoding UTF8 | ConvertFrom-Json
    $answer = $index.reply.'client-engine-configure'.'cmakeFiles-v1'
    $cmakeFiles = Get-Content -LiteralPath (Join-Path $replyDir $answer.jsonFile) -Raw -Encoding UTF8 |
        ConvertFrom-Json
    if ([IO.Path]::GetFullPath($cmakeFiles.paths.source) -ne [IO.Path]::GetFullPath($Root) -or
        [IO.Path]::GetFullPath($cmakeFiles.paths.build) -ne [IO.Path]::GetFullPath($BuildDir)) {
        throw 'CMake file API paths do not match the requested build directory.'
    }
    foreach ($inputFile in $cmakeFiles.inputs) {
        if ([IO.Path]::IsPathRooted($inputFile.path)) { $inputFile.path }
        else { Join-Path $Root $inputFile.path }
    }
}

function Invoke-ConfigurePreset {
    param([string]$Root, [string]$Preset, [string]$CMake, [bool]$Force, [bool]$Clean)

    $timer = [Diagnostics.Stopwatch]::StartNew()
    $buildDir = "$Root/out/build/$Preset"
    $statePath = "$buildDir/.engine-configure-state.json"
    $outputs = @("$buildDir/CMakeCache.txt", "$buildDir/vcpkg_installed/vcpkg/status")
    if ($Preset -eq 'default') {
        $outputs += @("$buildDir/Engine.sln", "$buildDir/Engine.vcxproj",
            "$buildDir/ALL_BUILD.vcxproj", "$buildDir/ZERO_CHECK.vcxproj",
            "$buildDir/CMakeFiles/generate.stamp")
    } else {
        $outputs += @("$buildDir/build.ninja", "$buildDir/CMakeFiles/rules.ninja", "$buildDir/compile_commands.json")
    }
    $reason = 'first run or missing configure state'
    if ($Clean) { $reason = 'reset requested' }
    elseif ($Force) { $reason = 'forced configure' }
    elseif (Test-Path -LiteralPath $statePath -PathType Leaf) {
        try {
            $state = Get-Content -LiteralPath $statePath -Raw -Encoding UTF8 | ConvertFrom-Json
            $missing = @($state.outputs | Where-Object { !(Test-Path -LiteralPath $_ -PathType Leaf) })
            if ($state.version -eq 1 -and $state.root -eq $Root -and $state.preset -eq $Preset -and
                $missing.Count -eq 0 -and $state.inputs.Count -gt 0 -and
                $state.fingerprint -eq (Get-ConfigureFingerprint $Root $state.inputs $state.outputs $CMake)) {
                Write-Host ('[SKIP] {0}: up to date ({1:N2}s).' -f $Preset, $timer.Elapsed.TotalSeconds)
                return 0
            }
            $reason = 'configuration, tools, file list or generated files changed'
        } catch { $reason = 'configure state cannot be verified' }
    }

    # Invalidate before CMake, so an interrupted/failed run is never cached.
    if (Test-Path -LiteralPath $statePath) { Remove-Item -LiteralPath $statePath -Force }
    $queryDir = "$buildDir/.cmake/api/v1/query/client-engine-configure"
    New-Item -ItemType Directory -Path $queryDir -Force | Out-Null
    New-Item -ItemType File -Path "$queryDir/cmakeFiles-v1" -Force | Out-Null
    $previousIndex = ''
    if (Test-Path -LiteralPath "$buildDir/.cmake/api/v1/reply") {
        $index = Get-ChildItem -LiteralPath "$buildDir/.cmake/api/v1/reply" -Filter 'index-*.json' |
            Sort-Object Name -Descending | Select-Object -First 1
        if ($index) { $previousIndex = $index.FullName }
    }
    Write-Host "[RUN] $Preset`: $reason."
    $cmakeArgs = @('--preset', $Preset)
    if ($Clean) { $cmakeArgs += '--fresh' }
    # Windows PowerShell can surface native stderr as errors; trust CMake's exit code.
    $savedPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        & $CMake @cmakeArgs | Out-Host
        $code = $LASTEXITCODE
    } finally { $ErrorActionPreference = $savedPreference }
    if ($code -ne 0) {
        Write-Host ("[ERROR] {0}: CMake exited with {1} ({2:N2}s)." -f $Preset, $code, $timer.Elapsed.TotalSeconds)
        Write-Host '        Fix the error above and retry; use --clean after changing toolchains.'
        return $code
    }
    try {
        $inputs = @(Get-ConfigureInputs $Root $buildDir $previousIndex)
        foreach ($path in $outputs) {
            if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing generated file: $path" }
        }
        if ($Preset -eq 'default') {
            $outputs += @(Get-ChildItem -LiteralPath $buildDir -File |
                Where-Object { $_.Extension -eq '.vcxproj' -or $_.Name -like '*.vcxproj.filters' } |
                ForEach-Object FullName)
        }
        $outputs += @(Get-ChildItem -LiteralPath "$buildDir/CMakeFiles" -Recurse -File -Filter '*.cmake' |
            ForEach-Object FullName)
        $state = [ordered]@{
            version = 1; root = $Root; preset = $Preset; inputs = $inputs; outputs = $outputs
            fingerprint = Get-ConfigureFingerprint $Root $inputs $outputs $CMake
        }
        $state | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath "$statePath.tmp" -Encoding UTF8
        Move-Item -LiteralPath "$statePath.tmp" -Destination $statePath -Force
    } catch {
        Write-Host "[WARN] Configure succeeded, but fast-path state could not be saved: $($_.Exception.Message)"
    }
    Write-Host ('[OK] {0}: configured ({1:N2}s).' -f $Preset, $timer.Elapsed.TotalSeconds)
    return 0
}

function Invoke-EngineConfigure {
    param([string[]]$Arguments)

    $force = $false
    $clean = $false
    $clangd = $false
    foreach ($argument in $Arguments) {
        switch ($argument.ToLowerInvariant()) {
            '--force' { $force = $true }
            '--clean' { $clean = $true }
            '--clangd' { $clangd = $true }
            '--help' {
                Write-Host 'Usage: configure.bat [--force] [--clean] [--clangd] [--help]'
                Write-Host '  Default: skip CMake when configuration and generated files are unchanged.'
                Write-Host '  --force   Run CMake even when up to date; keep the existing cache.'
                Write-Host '  --clean   Reset requested CMake caches with --fresh; keep installed dependencies.'
                Write-Host '  --clangd  Also generate the Ninja/MSVC clangd database (x64 VS tools prompt).'
                return 0
            }
            default { Write-Host "[ERROR] Unknown argument: $argument"; return 1 }
        }
    }
    $root = Split-Path $PSScriptRoot -Parent
    $timer = [Diagnostics.Stopwatch]::StartNew()
    try {
        $cmakeCommand = Get-Command cmake -CommandType Application -ErrorAction SilentlyContinue |
            Select-Object -First 1
        if (!$cmakeCommand) { throw 'CMake 3.25+ was not found in PATH.' }
        if (!$env:VCPKG_ROOT -or !(Test-Path -LiteralPath "$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" -PathType Leaf)) {
            throw 'Set VCPKG_ROOT to a vcpkg installation containing scripts/buildsystems/vcpkg.cmake.'
        }
        if ($clangd) {
            if (!(Get-Command ninja -CommandType Application -ErrorAction SilentlyContinue)) {
                throw 'Ninja was not found in PATH; it is required for --clangd.'
            }
            if (!(Get-Command cl -CommandType Application -ErrorAction SilentlyContinue) -or $env:VSCMD_ARG_TGT_ARCH -ne 'x64') {
                throw 'Run --clangd from an x64 Native Tools Command Prompt for VS 2022.'
            }
        }
        Push-Location -LiteralPath $root
        try {
            $lockDir = "$root/out/build"
            New-Item -ItemType Directory -Path $lockDir -Force | Out-Null
            try {
                $lock = [IO.File]::Open("$lockDir/.engine-configure.lock", 'OpenOrCreate', 'ReadWrite', 'None')
            } catch { throw 'Another configure is running, or out/build is not writable. Retry when it finishes.' }
            try {
                $code = Invoke-ConfigurePreset $root 'default' $cmakeCommand.Source $force $clean
                if ($code -ne 0) { return $code }
                if ($clangd) {
                    $code = Invoke-ConfigurePreset $root 'clangd' $cmakeCommand.Source $force $clean
                    if ($code -ne 0) { return $code }
                    Write-Host '[OK] clangd database: out/build/clangd/compile_commands.json'
                }
            } finally { $lock.Dispose() }
        } finally { Pop-Location }
        Write-Host ('[DONE] Total: {0:N2}s. Open out/build/default/Engine.sln; build: cmake --build --preset debug' -f $timer.Elapsed.TotalSeconds)
        return 0
    } catch { Write-Host "[ERROR] $($_.Exception.Message)"; return 1 }
}

# Dot-sourcing exposes functions to the regression tests without running CMake.
if ($MyInvocation.InvocationName -ne '.') {
    exit (Invoke-EngineConfigure -Arguments $args)
}
