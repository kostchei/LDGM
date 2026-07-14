$ErrorActionPreference = "Stop"

function Get-LDGMEnvironment {
    $repoRoot = Split-Path -Parent $PSScriptRoot
    $lock = Get-Content -Raw -LiteralPath (Join-Path $repoRoot "dependencies.lock.json") | ConvertFrom-Json
    $statePath = Join-Path $repoRoot ".bootstrap\dependencies.json"
    if (-not (Test-Path -LiteralPath $statePath)) {
        throw "Dependency state is missing. Run scripts/bootstrap-dependencies.ps1 first."
    }

    $state = Get-Content -Raw -LiteralPath $statePath | ConvertFrom-Json
    $dependencyRoot = $state.dependency_root
    $cmakeRoot = Join-Path $env:LOCALAPPDATA "LDGM\tools\$($lock.windows_toolchain.cmake_portable_directory)"
    $cmake = Join-Path $cmakeRoot "bin\cmake.exe"
    $o3de = Join-Path $dependencyRoot $lock.source_dependencies.o3de.checkout_directory
    $eigen = Join-Path $dependencyRoot $lock.source_dependencies.eigen.checkout_directory
    $chrono = Join-Path $dependencyRoot $lock.source_dependencies.project_chrono.checkout_directory

    foreach ($requiredPath in @($cmake, $o3de, $eigen, $chrono)) {
        if (-not (Test-Path -LiteralPath $requiredPath)) {
            throw "Required bootstrap path is missing: $requiredPath"
        }
    }

    return [pscustomobject]@{
        RepoRoot = $repoRoot
        DependencyRoot = $dependencyRoot
        O3DE = $o3de
        Eigen = $eigen
        Chrono = $chrono
        O3DEPackages = (Join-Path $dependencyRoot "o3de-packages")
        CMakeRoot = $cmakeRoot
        CMake = $cmake
    }
}

function Enter-LDGMEnvironment {
    $environment = Get-LDGMEnvironment
    $cmakeBin = Split-Path -Parent $environment.CMake
    if (($env:Path -split ";") -notcontains $cmakeBin) {
        $env:Path = "$cmakeBin;$env:Path"
    }
    return $environment
}
