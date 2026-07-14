[CmdletBinding()]
param(
    [string]$BuildDirectory = "build/windows"
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "common.ps1")
$environment = Enter-LDGMEnvironment

$userDirectory = Join-Path $environment.RepoRoot "user"
New-Item -ItemType Directory -Path $userDirectory -Force | Out-Null
$userProject = [ordered]@{
    engine_path = ($environment.O3DE -replace "\\", "/")
}
$userProject | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $userDirectory "project.json") -Encoding UTF8
New-Item -ItemType Directory -Path $environment.O3DEPackages -Force | Out-Null

$resolvedBuildDirectory = Join-Path $environment.RepoRoot $BuildDirectory
& $environment.CMake `
    -S $environment.RepoRoot `
    -B $resolvedBuildDirectory `
    -G "Visual Studio 17 2022" `
    -A x64 `
    "-DLY_3RDPARTY_PATH=$($environment.O3DEPackages)"

if ($LASTEXITCODE -ne 0) {
    throw "O3DE project configuration failed with exit code $LASTEXITCODE."
}

Write-Host "Configured LDGM in $resolvedBuildDirectory"
