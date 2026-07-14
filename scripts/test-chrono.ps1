[CmdletBinding()]
param(
    [ValidateSet("Debug", "Release", "RelWithDebInfo")]
    [string]$Configuration = "RelWithDebInfo"
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "common.ps1")
$environment = Enter-LDGMEnvironment

$chronoInstall = Join-Path $environment.DependencyRoot "chrono-install"
$chronoConfig = Join-Path $chronoInstall "cmake\ChronoConfig.cmake"
$smokeSource = Join-Path $environment.RepoRoot "tests\chrono_smoke"
$smokeBuild = Join-Path $environment.DependencyRoot "chrono-smoke-build"

if (-not (Test-Path -LiteralPath $chronoConfig)) {
    throw "Chrono is not installed at $chronoInstall. Run scripts\build-chrono.ps1 first."
}

& $environment.CMake -S $smokeSource -B $smokeBuild -G "Visual Studio 17 2022" -A x64 `
    "-DChrono_DIR=$(Split-Path -Parent $chronoConfig)"
if ($LASTEXITCODE -ne 0) {
    throw "Chrono smoke configuration failed with exit code $LASTEXITCODE."
}

& $environment.CMake --build $smokeBuild --config $Configuration --target ldgm_chrono_smoke -- "/m:1"
if ($LASTEXITCODE -ne 0) {
    throw "Chrono smoke build failed with exit code $LASTEXITCODE."
}

$ctest = Join-Path (Split-Path -Parent $environment.CMake) "ctest.exe"
& $ctest --test-dir $smokeBuild --build-config $Configuration --output-on-failure
if ($LASTEXITCODE -ne 0) {
    throw "Chrono smoke test failed with exit code $LASTEXITCODE."
}

Write-Host "Chrono Core/Vehicle standalone smoke test passed."
