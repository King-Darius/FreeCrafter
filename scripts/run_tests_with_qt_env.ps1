param(
    [string]$BuildDir = "build",
    [switch]$UseCTest,
    [string[]]$CTestArgs = @("--output-on-failure")
)

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Split-Path -Parent $scriptDir
$qtRoot = Join-Path $repoRoot "qt/6.5.3/msvc2019_64"
$qtBin = Join-Path $qtRoot "bin"
$qtPlugins = Join-Path $qtRoot "plugins"
$qtPlatformPlugins = Join-Path $qtPlugins "platforms"

$cmdTool = Get-Command cmd.exe -ErrorAction SilentlyContinue
if (-not $cmdTool) {
    Write-Error "cmd.exe was not found on PATH. Launch the script from a standard Windows shell or ensure System32 is available." -ErrorAction Stop
}
$ctestTool = Get-Command ctest -ErrorAction SilentlyContinue
if ($UseCTest -and -not $ctestTool) {
    Write-Error "CTest was not found on PATH. Run from a Visual Studio Developer Prompt or add CMake's bin directory to PATH." -ErrorAction Stop
}

if (-not (Test-Path $qtBin)) {
    Write-Error "Qt runtime not found at $qtBin. Run scripts/bootstrap.py first." -ErrorAction Stop
}
if (-not (Test-Path $qtPlugins)) {
    Write-Error "Qt plugins not found at $qtPlugins. Run scripts/bootstrap.py first." -ErrorAction Stop
}
if (-not (Test-Path $qtPlatformPlugins)) {
    Write-Error "Qt platform plugins not found at $qtPlatformPlugins. Run scripts/bootstrap.py first." -ErrorAction Stop
}

$buildPath = if ([System.IO.Path]::IsPathRooted($BuildDir)) { $BuildDir } else { Join-Path $repoRoot $BuildDir }
if (-not (Test-Path $buildPath)) {
    Write-Error "Build directory '$buildPath' does not exist." -ErrorAction Stop
}
$envPath = $env:PATH
$commonEnvCommands = [System.Collections.Generic.List[string]]::new()
$commonEnvCommands.Add("set `"PATH=$qtBin;$envPath`"")
$commonEnvCommands.Add("set `"QT_PLUGIN_PATH=$qtPlugins`"")
$commonEnvCommands.Add("set `"QT_QPA_PLATFORM_PLUGIN_PATH=$qtPlatformPlugins`"")
$commonEnvCommands.Add("set `"QT_QPA_PLATFORM=windows`"")

if ($UseCTest) {
    $ctestArgsList = [System.Collections.Generic.List[string]]::new()
    if ($CTestArgs -and $CTestArgs.Count -gt 0) {
        foreach ($arg in $CTestArgs) {
            $trimmed = [string]$arg
            if (-not [string]::IsNullOrWhiteSpace($trimmed)) {
                $ctestArgsList.Add($trimmed)
            }
        }
    } else {
        $ctestArgsList.Add("--output-on-failure")
    }

    $hasConfigArg = $false
    foreach ($arg in $ctestArgsList) {
        if ($arg -eq "-C" -or $arg -like "-C=*" -or $arg -eq "--build-config" -or $arg -like "--build-config=*") {
            $hasConfigArg = $true
            break
        }
    }
    if (-not $hasConfigArg) {
        $config = $env:CTEST_CONFIGURATION_TYPE
        if (-not $config) {
            $cacheFile = Join-Path $buildPath "CMakeCache.txt"
            if (Test-Path $cacheFile) {
                $hasMultiConfig = Select-String -Path $cacheFile -Pattern '^CMAKE_CONFIGURATION_TYPES:' -ErrorAction SilentlyContinue
                if ($hasMultiConfig) {
                    $config = "Release"
                }
            }
        }
        if ($config) {
            $ctestArgsList.Add("-C")
            $ctestArgsList.Add($config)
        }
    }

    $args = [string]::Join(' ', $ctestArgsList)
    $envCommands = [System.Collections.Generic.List[string]]::new()
    foreach ($cmd in $commonEnvCommands) {
        $envCommands.Add($cmd)
    }
    $envCommands.Add("set `"QT_OPENGL=angle`"")
    $envCommands.Add("set `"FREECRAFTER_RENDER_SKIP_COVERAGE=1`"")
    $envCommands.Add("set `"QT_DEBUG_PLUGINS=1`"")
    $command = "{0} && ctest {1}" -f ([string]::Join(' && ', $envCommands)), $args
} else {
    $binary = Join-Path $buildPath "test_render.exe"
    if (-not (Test-Path $binary)) {
        Write-Error "Test binary not found at $binary" -ErrorAction Stop
    }
    $command = "{0} && `"{1}`"" -f ([string]::Join(' && ', $commonEnvCommands)), $binary
}
Push-Location $buildPath
try {
    Write-Host "cmd.exe /C $command"
    & cmd.exe /C $command
    exit $LASTEXITCODE
}
finally {
    Pop-Location
}
