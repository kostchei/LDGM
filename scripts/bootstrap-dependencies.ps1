[CmdletBinding()]
param(
    [string]$DependencyRoot = (Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) "LDGM-deps")
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$lock = Get-Content -Raw -LiteralPath (Join-Path $repoRoot "dependencies.lock.json") | ConvertFrom-Json

function Get-LockedSource {
    param(
        [Parameter(Mandatory = $true)]$Dependency,
        [Parameter(Mandatory = $true)][string]$Root
    )

    $destination = Join-Path $Root $Dependency.checkout_directory
    if (-not (Test-Path -LiteralPath $destination)) {
        Write-Host "Cloning $($Dependency.repository) into $destination"
        & git clone --filter=blob:none --no-checkout $Dependency.repository $destination
        if ($LASTEXITCODE -ne 0) {
            throw "Clone failed for $($Dependency.repository)."
        }
    }

    if (-not (Test-Path -LiteralPath (Join-Path $destination ".git"))) {
        throw "$destination exists but is not a Git checkout."
    }

    & git -C $destination fetch --depth 1 origin $Dependency.commit
    if ($LASTEXITCODE -ne 0) {
        throw "Fetch failed for $($Dependency.repository) at $($Dependency.commit)."
    }
    & git -C $destination checkout --detach $Dependency.commit
    if ($LASTEXITCODE -ne 0) {
        throw "Checkout failed for $($Dependency.repository) at $($Dependency.commit)."
    }

    $actualCommit = (& git -C $destination rev-parse HEAD).Trim()
    if ($actualCommit -ne $Dependency.commit) {
        throw "Dependency commit mismatch at ${destination}: expected $($Dependency.commit), got $actualCommit."
    }

    Write-Host "Locked $destination at $actualCommit"
}

New-Item -ItemType Directory -Path $DependencyRoot -Force | Out-Null
Get-LockedSource -Dependency $lock.source_dependencies.o3de -Root $DependencyRoot
Get-LockedSource -Dependency $lock.source_dependencies.project_chrono -Root $DependencyRoot

$state = [ordered]@{
    created_utc = [DateTime]::UtcNow.ToString("o")
    dependency_root = (Resolve-Path $DependencyRoot).Path
    o3de_commit = $lock.source_dependencies.o3de.commit
    chrono_commit = $lock.source_dependencies.project_chrono.commit
}
$stateDir = Join-Path $repoRoot ".bootstrap"
New-Item -ItemType Directory -Path $stateDir -Force | Out-Null
$state | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath (Join-Path $stateDir "dependencies.json") -Encoding UTF8

Write-Host "Dependency bootstrap complete."
