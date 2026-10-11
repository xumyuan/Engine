# Run with: powershell -NoProfile -File scripts/Test-Configure.ps1
# Portable regression tests; CMake/VS/vcpkg are simulated in a temporary tree.
param([string]$CMakeExecutable = '')

. "$PSScriptRoot/Configure.ps1"

function Assert-Equal {
    param($Actual, $Expected, [string]$Message)
    if ($Actual -ne $Expected) { throw "$Message`: expected $Expected, got $Actual" }
    $script:Assertions++
}

function Write-TestFile {
    param([string]$Path, [string]$Value = 'test')
    New-Item -ItemType Directory -Path (Split-Path $Path -Parent) -Force | Out-Null
    Set-Content -LiteralPath $Path -Value $Value -Encoding UTF8
}

function Invoke-TestCMake {
    $script:Calls++
    $script:LastArgs = @($args)
    $global:LASTEXITCODE = $script:FailureCode
    Write-Output 'Simulated native CMake output'
    if ($script:FailureCode -ne 0) { return }
    $preset = $args[1]
    $buildDir = "$script:TestRoot/out/build/$preset"
    foreach ($relative in @('CMakeCache.txt', 'vcpkg_installed/vcpkg/status')) {
        Write-TestFile "$buildDir/$relative"
    }
    if ($preset -eq 'default') {
        foreach ($relative in @('Engine.sln', 'Engine.vcxproj', 'Engine.vcxproj.filters',
            'ALL_BUILD.vcxproj', 'ZERO_CHECK.vcxproj', 'CMakeFiles/generate.stamp')) {
            Write-TestFile "$buildDir/$relative"
        }
    } else {
        Write-TestFile "$buildDir/build.ninja"
        Write-TestFile "$buildDir/CMakeFiles/rules.ninja"
        Write-TestFile "$buildDir/compile_commands.json" '[]'
    }
    $reply = "$buildDir/.cmake/api/v1/reply"
    $cmakeFiles = @{
        paths = @{ source = $script:TestRoot; build = $buildDir }
        inputs = @(@{ path = 'CMakeLists.txt' }, @{ path = "$script:TestRoot/toolchain.cmake" })
    }
    Write-TestFile "$reply/cmakeFiles.json" ($cmakeFiles | ConvertTo-Json -Depth 5)
    $index = @{ reply = @{ 'client-engine-configure' = @{ 'cmakeFiles-v1' = @{ jsonFile = 'cmakeFiles.json' } } } }
    Write-TestFile ("$reply/index-{0:D4}.json" -f $script:Calls) ($index | ConvertTo-Json -Depth 5)
}

function Run-TestPreset {
    param([string]$Preset = 'default', [bool]$Force = $false, [bool]$Clean = $false)
    return Invoke-ConfigurePreset $script:TestRoot $Preset 'Invoke-TestCMake' $Force $Clean
}

$script:Assertions = 0
$script:Calls = 0
$script:FailureCode = 0
$script:LastArgs = @()
$script:TestRoot = Join-Path ([IO.Path]::GetTempPath()) ("engine configure " + [char]0x6D4B + ' ' + [guid]::NewGuid())
$savedOverlay = $env:VCPKG_OVERLAY_PORTS
$savedCxxFlags = $env:CXXFLAGS
try {
    Write-TestFile "$script:TestRoot/CMakeLists.txt" 'project(Test)'
    Write-TestFile "$script:TestRoot/toolchain.cmake"
    Write-TestFile "$script:TestRoot/configure.bat"
    Write-TestFile "$script:TestRoot/scripts/Configure.ps1"
    Write-TestFile "$script:TestRoot/CMakePresets.json" '{"version":6}'
    Write-TestFile "$script:TestRoot/vcpkg.json" '{"name":"test"}'
    Write-TestFile "$script:TestRoot/vcpkg-configuration.json" '{"overlay-ports":["ports"]}'
    Write-TestFile "$script:TestRoot/ports/test/portfile.cmake"
    Write-TestFile "$script:TestRoot/Source/main.cpp"
    Write-TestFile "$script:TestRoot/Source/shaders/main.glsl"
    $env:VCPKG_OVERLAY_PORTS = ''
    $env:CXXFLAGS = ''

    Assert-Equal (Run-TestPreset) 0 'First configure succeeds and returns only its exit code'
    Assert-Equal $script:Calls 1 'First configure invokes CMake'
    Assert-Equal (Run-TestPreset) 0 'Unchanged configure succeeds'
    Assert-Equal $script:Calls 1 'Unchanged configure skips CMake'
    Write-TestFile "$script:TestRoot/Source/main.cpp" 'changed source content'
    Write-TestFile "$script:TestRoot/Source/shaders/main.glsl" 'changed shader content'
    Run-TestPreset | Out-Null
    Assert-Equal $script:Calls 1 'Editing source/shader content skips CMake'

    Write-TestFile "$script:TestRoot/Source/added.h"
    Run-TestPreset | Out-Null
    Assert-Equal $script:Calls 2 'Adding a globbed file configures'
    Remove-Item -LiteralPath "$script:TestRoot/Source/added.h"
    Run-TestPreset | Out-Null
    Assert-Equal $script:Calls 3 'Removing a globbed file configures'

    $stamp = (Get-Item -LiteralPath "$script:TestRoot/toolchain.cmake").LastWriteTimeUtc
    Write-TestFile "$script:TestRoot/toolchain.cmake" 'TEST'
    (Get-Item -LiteralPath "$script:TestRoot/toolchain.cmake").LastWriteTimeUtc = $stamp
    Run-TestPreset | Out-Null
    Assert-Equal $script:Calls 4 'Content hashing detects a toolchain change with the same size and timestamp'
    Write-TestFile "$script:TestRoot/vcpkg.json" '{"name":"changed"}'
    Run-TestPreset | Out-Null
    Assert-Equal $script:Calls 5 'Manifest changes configure'
    Write-TestFile "$script:TestRoot/ports/test/portfile.cmake" 'changed overlay'
    Run-TestPreset | Out-Null
    Assert-Equal $script:Calls 6 'Overlay changes configure'

    Write-TestFile "$script:TestRoot/external ports/test/portfile.cmake"
    $env:VCPKG_OVERLAY_PORTS = "$script:TestRoot/external ports"
    Run-TestPreset | Out-Null
    Assert-Equal $script:Calls 7 'Absolute overlay paths configure'
    Write-TestFile "$script:TestRoot/external ports/test/portfile.cmake" 'changed external overlay'
    Run-TestPreset | Out-Null
    Assert-Equal $script:Calls 8 'External overlay edits configure'
    $env:CXXFLAGS = '/DTEST'
    Run-TestPreset | Out-Null
    Assert-Equal $script:Calls 9 'Compiler environment changes configure'

    Write-TestFile "$script:TestRoot/CMakePresets.json" '{"version":6,"include":["extra.json"]}'
    Write-TestFile "$script:TestRoot/extra.json" '{"version":6}'
    Run-TestPreset | Out-Null
    Assert-Equal $script:Calls 10 'Adding a preset include configures'
    Write-TestFile "$script:TestRoot/extra.json" '{"version":5}'
    Run-TestPreset | Out-Null
    Assert-Equal $script:Calls 11 'Included preset changes configure'

    $statePath = "$script:TestRoot/out/build/default/.engine-configure-state.json"
    Write-TestFile $statePath 'not json'
    Run-TestPreset | Out-Null
    Assert-Equal $script:Calls 12 'Corrupt state recovers through configure'
    Remove-Item -LiteralPath "$script:TestRoot/out/build/default/Engine.vcxproj.filters"
    Run-TestPreset | Out-Null
    Assert-Equal $script:Calls 13 'Missing generated file configures'
    Run-TestPreset -Force $true | Out-Null
    Assert-Equal $script:Calls 14 'Force configures even when unchanged'
    Assert-Equal ($script:LastArgs -contains '--fresh') $false 'Force keeps the CMake cache'
    Run-TestPreset -Clean $true | Out-Null
    Assert-Equal $script:Calls 15 'Clean configures even when unchanged'
    Assert-Equal ($script:LastArgs -contains '--fresh') $true 'Clean passes --fresh'

    $script:FailureCode = 23
    Assert-Equal (Run-TestPreset -Force $true) 23 'Native failure exit code is propagated'
    Assert-Equal (Test-Path -LiteralPath $statePath) $false 'Failed configure invalidates prior success'
    $script:FailureCode = 0
    Run-TestPreset | Out-Null
    Assert-Equal $script:Calls 17 'Failure recovery invokes CMake'
    Run-TestPreset | Out-Null
    Assert-Equal $script:Calls 17 'Recovered configure can skip'

    Run-TestPreset -Preset 'clangd' | Out-Null
    Assert-Equal $script:Calls 18 'Clangd configures independently'
    Run-TestPreset | Out-Null
    Run-TestPreset -Preset 'clangd' | Out-Null
    Assert-Equal $script:Calls 18 'Both presets skip independently'
    Remove-Item -LiteralPath "$script:TestRoot/out/build/clangd/compile_commands.json"
    Run-TestPreset -Preset 'clangd' | Out-Null
    Assert-Equal $script:Calls 19 'Missing clangd database configures'

    $lockPath = "$script:TestRoot/out/build/.test.lock"
    $lock = [IO.File]::Open($lockPath, 'OpenOrCreate', 'ReadWrite', 'None')
    try {
        $refused = $false
        try { $other = [IO.File]::Open($lockPath, 'OpenOrCreate', 'ReadWrite', 'None'); $other.Dispose() }
        catch { $refused = $true }
        Assert-Equal $refused $true 'An exclusive configure lock rejects a second writer'
    } finally { $lock.Dispose() }
    $lock = [IO.File]::Open($lockPath, 'OpenOrCreate', 'ReadWrite', 'None')
    $lock.Dispose()
    Assert-Equal (Invoke-EngineConfigure @('--bad-option')) 1 'Unknown options fail'
    Assert-Equal (Invoke-EngineConfigure @('--help')) 0 'Help does not require build tools'

    if ($CMakeExecutable) {
        Write-TestFile "$script:TestRoot/CMakeLists.txt" "cmake_minimum_required(VERSION 3.25)`nproject(Test LANGUAGES NONE)"
        $realBuild = "$script:TestRoot/real-file-api"
        Write-TestFile "$realBuild/.cmake/api/v1/query/client-engine-configure/cmakeFiles-v1" ''
        & $CMakeExecutable -S $script:TestRoot -B $realBuild | Out-Host
        Assert-Equal $LASTEXITCODE 0 'Real CMake configure succeeds'
        $realInputs = @(Get-ConfigureInputs $script:TestRoot $realBuild '')
        Assert-Equal ($realInputs -contains "$script:TestRoot/CMakeLists.txt") $true 'Real file API returns source inputs'
        Assert-Equal ($realInputs.Count -gt 1) $true 'Real file API includes external CMake modules'
        $hash = Get-ConfigureFingerprint $script:TestRoot $realInputs @() $CMakeExecutable
        Assert-Equal $hash.Length 64 'Real file API inputs can be fingerprinted'
        & $CMakeExecutable -S $script:TestRoot -B $realBuild --fresh | Out-Host
        Assert-Equal $LASTEXITCODE 0 'Real CMake --fresh succeeds'
        Assert-Equal (@(Get-ConfigureInputs $script:TestRoot $realBuild '').Count -gt 1) $true '--fresh retains the file API query'
    }
    Write-Host "[PASS] $script:Assertions assertions."
} finally {
    $env:VCPKG_OVERLAY_PORTS = $savedOverlay
    $env:CXXFLAGS = $savedCxxFlags
    if (Test-Path -LiteralPath $script:TestRoot) { Remove-Item -LiteralPath $script:TestRoot -Recurse -Force }
}
