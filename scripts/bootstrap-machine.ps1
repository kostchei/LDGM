[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [switch]$InstallMissing
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$lock = Get-Content -Raw -LiteralPath (Join-Path $repoRoot "dependencies.lock.json") | ConvertFrom-Json

$preflight = & (Join-Path $PSScriptRoot "preflight.ps1") -PassThru
if ($preflight.ready) {
    Write-Host "Machine prerequisites are already satisfied."
    exit 0
}

Write-Host "Missing prerequisites:"
if (-not $preflight.visual_studio.found) { Write-Host "  - Visual Studio 2022 C++ toolchain" }
if (-not $preflight.cmake.found) { Write-Host "  - CMake $($lock.windows_toolchain.cmake_version)" }
if (-not $preflight.git.found) { Write-Host "  - Git" }
if (-not $preflight.git_lfs.found) { Write-Host "  - Git LFS" }

if (-not $InstallMissing) {
    Write-Host "Prerequisites are missing. Re-run with -InstallMissing from an elevated PowerShell session."
    exit 2
}

$winget = Get-Command winget -ErrorAction SilentlyContinue
if (-not $winget) {
    throw "winget is unavailable. Install Visual Studio 2022 and CMake $($lock.windows_toolchain.cmake_version) manually, then rerun preflight.ps1."
}

$vsConfig = Join-Path $repoRoot $lock.windows_toolchain.visual_studio_config
$vsProduct = $lock.windows_toolchain.visual_studio_product
if (-not $preflight.visual_studio.found -and $PSCmdlet.ShouldProcess($vsProduct, "Install Visual Studio with $vsConfig")) {
    & winget install --id $vsProduct --exact --source winget --accept-package-agreements --accept-source-agreements --override "--wait --passive --norestart --config `"$vsConfig`""
    if ($LASTEXITCODE -ne 0) {
        throw "Visual Studio installation failed with exit code $LASTEXITCODE."
    }
}

$cmakeVersion = $lock.windows_toolchain.cmake_version
$cmakeDirectory = $lock.windows_toolchain.cmake_portable_directory
$toolsRoot = Join-Path $env:LOCALAPPDATA "LDGM\tools"
$cmakeRoot = Join-Path $toolsRoot $cmakeDirectory
$cmakeExecutable = Join-Path $cmakeRoot "bin\cmake.exe"
if (-not (Test-Path -LiteralPath $cmakeExecutable) -and $PSCmdlet.ShouldProcess("CMake $cmakeVersion", "Install pinned portable archive")) {
    $downloadRoot = Join-Path $repoRoot ".bootstrap\downloads"
    $archivePath = Join-Path $downloadRoot "$cmakeDirectory.zip"
    New-Item -ItemType Directory -Path $downloadRoot -Force | Out-Null
    New-Item -ItemType Directory -Path $toolsRoot -Force | Out-Null

    Write-Host "Downloading portable CMake $cmakeVersion"
    Invoke-WebRequest -UseBasicParsing -Uri $lock.windows_toolchain.cmake_portable_archive -OutFile $archivePath
    $actualHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $archivePath).Hash.ToLowerInvariant()
    if ($actualHash -ne $lock.windows_toolchain.cmake_portable_sha256) {
        throw "CMake archive hash mismatch: expected $($lock.windows_toolchain.cmake_portable_sha256), got $actualHash."
    }

    Expand-Archive -LiteralPath $archivePath -DestinationPath $toolsRoot -Force
    if (-not (Test-Path -LiteralPath $cmakeExecutable)) {
        throw "Portable CMake installation did not create $cmakeExecutable."
    }
}

$finalPreflight = & (Join-Path $PSScriptRoot "preflight.ps1") -PassThru
if (-not $finalPreflight.ready) {
    throw "Installation commands completed but the machine still fails preflight."
}

Write-Host "Machine bootstrap complete."
