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

Copy-Item -LiteralPath (Join-Path $environment.RepoRoot "cmake\Directory.Build.targets") `
    -Destination (Join-Path $resolvedBuildDirectory "Directory.Build.targets") -Force
$disableCompilerBatching = if ($MaxCompilerJobs -eq 1) { "true" } else { "false" }

& $environment.CMake --build $resolvedBuildDirectory --config $Configuration --target @Targets -- `
    "/m:$MaxParallelJobs" "/p:CL_MPCount=$MaxCompilerJobs" `
    "/p:LDGMDisableCompilerBatching=$disableCompilerBatching"
if ($LASTEXITCODE -ne 0) {
    throw "O3DE project build failed with exit code $LASTEXITCODE."
}

Write-Host "Built $($Targets -join ', ') ($Configuration)."
