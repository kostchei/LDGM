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
$chronoConfigDirectory = Join-Path $environment.DependencyRoot "chrono-install\cmake"
if (-not (Test-Path -LiteralPath (Join-Path $chronoConfigDirectory "ChronoConfig.cmake"))) {
    throw "Chrono is not installed. Run scripts\build-chrono.ps1 first."
}
$chronoConfigDirectoryCMake = $chronoConfigDirectory.Replace("\", "/")

& $environment.CMake `
    -S $environment.RepoRoot `
    -B $resolvedBuildDirectory `
    -G "Visual Studio 17 2022" `
    -A x64 `
    "-DLY_3RDPARTY_PATH=$($environment.O3DEPackages)" `
    "-DChrono_DIR=$chronoConfigDirectoryCMake"

if ($LASTEXITCODE -ne 0) {
    throw "O3DE project configuration failed with exit code $LASTEXITCODE."
}

Write-Host "Configured LDGM in $resolvedBuildDirectory"
