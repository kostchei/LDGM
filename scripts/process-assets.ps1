[CmdletBinding()]
param(
    [ValidateSet("debug", "profile", "release")]
    [string]$Configuration = "profile",
    [ValidateSet("pc")]
    [string]$Platform = "pc",
    [string]$BuildDirectory = "build/windows"
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "common.ps1")
$environment = Enter-LDGMEnvironment
$processor = Join-Path $environment.RepoRoot "$BuildDirectory\bin\$Configuration\AssetProcessorBatch.exe"

if (-not (Test-Path -LiteralPath $processor)) {
    throw "AssetProcessorBatch is not built. Run scripts/build-project.ps1 -Targets AssetProcessorBatch first."
}

& $processor "--engine-path=$($environment.O3DE)" "--project-path=$($environment.RepoRoot)" "/platform=$Platform"
if ($LASTEXITCODE -ne 0) {
    throw "AssetProcessorBatch failed with exit code $LASTEXITCODE."
}

Write-Host "Processed LDGM assets for $Platform ($Configuration tools)."
