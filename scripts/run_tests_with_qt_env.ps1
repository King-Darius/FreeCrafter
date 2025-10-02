param(
    [string]$BuildDir = "build",
    [switch]$UseCTest,
    [string[]]$CTestArgs = @("--output-on-failure")
)

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Split-Path -Parent $scriptDir
$qtBin = Join-Path $repoRoot "qt/6.5.3/msvc2019_64/bin"
if (-not (Test-Path $qtBin)) {
    Write-Error "Qt runtime not found at $qtBin. Run scripts/bootstrap.py first." -ErrorAction Stop
}
$buildPath = if ([System.IO.Path]::IsPathRooted($BuildDir)) { $BuildDir } else { Join-Path $repoRoot $BuildDir }
if (-not (Test-Path $buildPath)) {
    Write-Error "Build directory '$buildPath' does not exist." -ErrorAction Stop
}
$envPath = $env:PATH
if ($UseCTest) {
    $args = if ($CTestArgs) { $CTestArgs -join ' ' } else { '' }
    $command = "set `"PATH=$qtBin;$envPath`" && ctest $args"
} else {
    $binary = Join-Path $buildPath "test_render.exe"
    if (-not (Test-Path $binary)) {
        Write-Error "Test binary not found at $binary" -ErrorAction Stop
    }
    $command = "set `"PATH=$qtBin;$envPath`" && `"$binary`""
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
