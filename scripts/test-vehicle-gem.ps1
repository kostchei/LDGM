[CmdletBinding()]
param(
    [ValidateSet("debug", "profile", "release")]
    [string]$Configuration = "profile",
    [string]$BuildDirectory = "build/windows"
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "common.ps1")
$environment = Enter-LDGMEnvironment

& (Join-Path $PSScriptRoot "build-project.ps1") `
    -Configuration $Configuration `
    -BuildDirectory $BuildDirectory `
    -Targets "LDMChronoVehicle.Tests"

$ctest = Join-Path (Split-Path -Parent $environment.CMake) "ctest.exe"
$resolvedBuildDirectory = Join-Path $environment.RepoRoot $BuildDirectory
& $ctest --test-dir $resolvedBuildDirectory -C $Configuration -R "LDMChronoVehicle" --output-on-failure
if ($LASTEXITCODE -ne 0) {
    throw "LDMChronoVehicle tests failed with exit code $LASTEXITCODE."
}

Write-Host "LDMChronoVehicle unit tests passed ($Configuration)."
