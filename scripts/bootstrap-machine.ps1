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
if ($PSCmdlet.ShouldProcess($vsProduct, "Install Visual Studio with $vsConfig")) {
    & winget install --id $vsProduct --exact --source winget --accept-package-agreements --accept-source-agreements --override "--wait --passive --norestart --config `"$vsConfig`""
    if ($LASTEXITCODE -ne 0) {
        throw "Visual Studio installation failed with exit code $LASTEXITCODE."
    }
}

$cmakeProduct = $lock.windows_toolchain.cmake_product
$cmakeVersion = $lock.windows_toolchain.cmake_version
if ($PSCmdlet.ShouldProcess("$cmakeProduct $cmakeVersion", "Install CMake")) {
    & winget install --id $cmakeProduct --exact --version $cmakeVersion --source winget --accept-package-agreements --accept-source-agreements --silent
    if ($LASTEXITCODE -ne 0) {
        throw "CMake installation failed with exit code $LASTEXITCODE."
    }
}

Write-Host "Installation commands completed. Open a new PowerShell session and run scripts/preflight.ps1."
