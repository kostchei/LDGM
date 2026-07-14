[CmdletBinding()]
param(
    [switch]$Json,
    [switch]$PassThru
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$lockPath = Join-Path $repoRoot "dependencies.lock.json"
$lock = Get-Content -Raw -LiteralPath $lockPath | ConvertFrom-Json
$portableCmake = Join-Path $env:LOCALAPPDATA "LDGM\tools\$($lock.windows_toolchain.cmake_portable_directory)\bin\cmake.exe"

function Resolve-CommandVersion {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string[]]$VersionArguments
    )

    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if (-not $command) {
        return [ordered]@{ found = $false; path = $null; version = $null }
    }

    $versionOutput = (& $command.Source @VersionArguments 2>&1 | Select-Object -First 1).ToString().Trim()
    return [ordered]@{ found = $true; path = $command.Source; version = $versionOutput }
}

$vswhereCandidates = @(
    "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe",
    "$env:ProgramFiles\Microsoft Visual Studio\Installer\vswhere.exe"
)
$vswhere = $vswhereCandidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
$visualStudio = [ordered]@{ found = $false; path = $null; version = $null }
if ($vswhere) {
    $requiredVsMajor = [int]$lock.windows_toolchain.visual_studio_major_version
    $versionRange = "[$requiredVsMajor.0,$($requiredVsMajor + 1).0)"
    $installationJson = & $vswhere -latest -products * -version $versionRange -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -format json
    if ($LASTEXITCODE -eq 0 -and $installationJson) {
        $installation = $installationJson | ConvertFrom-Json | Select-Object -First 1
        if ($installation) {
            $visualStudio = [ordered]@{
                found = $true
                path = $installation.installationPath
                version = $installation.installationVersion
            }
        }
    }
}

$os = Get-CimInstance Win32_OperatingSystem
$disk = Get-CimInstance Win32_LogicalDisk -Filter "DeviceID='$((Get-Item $repoRoot).PSDrive.Name):'"
$result = [ordered]@{
    ready = $false
    repository = $repoRoot
    operating_system = [ordered]@{
        caption = $os.Caption
        version = $os.Version
        architecture = $os.OSArchitecture
    }
    disk_free_gb = [math]::Round($disk.FreeSpace / 1GB, 1)
    git = Resolve-CommandVersion -Name "git" -VersionArguments @("--version")
    git_lfs = Resolve-CommandVersion -Name "git-lfs" -VersionArguments @("version")
    winget = Resolve-CommandVersion -Name "winget" -VersionArguments @("--version")
    cmake = Resolve-CommandVersion -Name $(if (Test-Path -LiteralPath $portableCmake) { $portableCmake } else { "cmake" }) -VersionArguments @("--version")
    visual_studio = $visualStudio
    requirements = [ordered]@{
        cmake_version = $lock.windows_toolchain.cmake_version
        visual_studio_major_version = $lock.windows_toolchain.visual_studio_major_version
        minimum_free_disk_gb = 250
    }
}

$result.ready = (
    $result.git.found -and
    $result.git_lfs.found -and
    $result.cmake.found -and
    $result.cmake.version -eq "cmake version $($lock.windows_toolchain.cmake_version)" -and
    $result.visual_studio.found -and
    $result.disk_free_gb -ge $result.requirements.minimum_free_disk_gb
)

if ($PassThru) {
    return [pscustomobject]$result
} elseif ($Json) {
    $result | ConvertTo-Json -Depth 6
} else {
    Write-Host "LDGM Windows preflight"
    Write-Host "  Repository:       $($result.repository)"
    Write-Host "  OS:               $($result.operating_system.caption) $($result.operating_system.version)"
    Write-Host "  Free disk:        $($result.disk_free_gb) GB"
    Write-Host "  Git:              $($result.git.found) $($result.git.version)"
    Write-Host "  Git LFS:          $($result.git_lfs.found) $($result.git_lfs.version)"
    Write-Host "  CMake:            $($result.cmake.found) $($result.cmake.version)"
    Write-Host "  Visual Studio:    $($result.visual_studio.found) $($result.visual_studio.version)"
    Write-Host "  Ready:            $($result.ready)"
}

if (-not $result.ready) {
    exit 2
}

exit 0
