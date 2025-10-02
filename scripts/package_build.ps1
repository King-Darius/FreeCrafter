param(
    [string]$BuildDir = "build",
    [string]$InstallPrefix = "dist",
    [switch]$SkipInstall,
    [switch]$SkipPackage
)

$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$qtBin = Join-Path $repoRoot "qt/6.5.3/msvc2019_64/bin"
if (-not (Test-Path $qtBin)) {
    Write-Error "Qt runtime not found at $qtBin. Run scripts/bootstrap.py first." -ErrorAction Stop
}
$buildPath = Join-Path $repoRoot $BuildDir
if (-not (Test-Path $buildPath)) {
    Write-Error "Build directory '$buildPath' does not exist." -ErrorAction Stop
}
$env:PATH = "$qtBin;$($env:PATH)"
Push-Location $buildPath
try {
    & cmake --build .
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    if (-not $SkipInstall) {
        $prefixPath = if ([System.IO.Path]::IsPathRooted($InstallPrefix)) { $InstallPrefix } else { Join-Path $repoRoot $InstallPrefix }
        & cmake --install . --prefix "$prefixPath"
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }

    if (-not $SkipPackage) {
        & cmake --build . --target package
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
finally {
    Pop-Location
}
