[CmdletBinding()]
param(
    [ValidateSet("debug", "profile", "release")]
    [string]$Configuration = "profile",
    [string[]]$Targets = @("LDGM.GameLauncher", "LDGM.ServerLauncher"),
    [string]$BuildDirectory = "build/windows",
    [ValidateRange(1, 32)]
    [int]$MaxParallelJobs = 1,
    [ValidateRange(1, 32)]
    [int]$MaxCompilerJobs = 1
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "common.ps1")
$environment = Enter-LDGMEnvironment
$resolvedBuildDirectory = Join-Path $environment.RepoRoot $BuildDirectory

if (-not (Test-Path -LiteralPath (Join-Path $resolvedBuildDirectory "CMakeCache.txt"))) {
    throw "The project is not configured. Run scripts/configure-project.ps1 first."
}

& $environment.CMake --build $resolvedBuildDirectory --config $Configuration --target @Targets -- "/m:$MaxParallelJobs" "/p:CL_MPCount=$MaxCompilerJobs"
if ($LASTEXITCODE -ne 0) {
    throw "O3DE project build failed with exit code $LASTEXITCODE."
}

Write-Host "Built $($Targets -join ', ') ($Configuration)."
