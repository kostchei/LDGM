[CmdletBinding()]
param(
    [ValidateSet("Debug", "Release", "RelWithDebInfo")]
    [string]$Configuration = "RelWithDebInfo",
    [ValidateRange(1, 32)]
    [int]$MaxParallelJobs = 1,
    [ValidateRange(1, 32)]
    [int]$MaxCompilerJobs = 1
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "common.ps1")
$environment = Enter-LDGMEnvironment

$eigenBuild = Join-Path $environment.DependencyRoot "eigen-build"
$eigenInstall = Join-Path $environment.DependencyRoot "eigen-install"
$chronoBuild = Join-Path $environment.DependencyRoot "chrono-build"
$chronoInstall = Join-Path $environment.DependencyRoot "chrono-install"

& $environment.CMake -S $environment.Eigen -B $eigenBuild -G "Visual Studio 17 2022" -A x64 `
    -DBUILD_TESTING=OFF `
    -DEIGEN_BUILD_TESTING=OFF `
    -DEIGEN_BUILD_BLAS=OFF `
    -DEIGEN_BUILD_LAPACK=OFF `
    -DEIGEN_BUILD_DOC=OFF `
    -DEIGEN_BUILD_DEMOS=OFF `
    "-DCMAKE_INSTALL_PREFIX=$eigenInstall"
if ($LASTEXITCODE -ne 0) {
    throw "Eigen configuration failed with exit code $LASTEXITCODE."
}

& $environment.CMake --install $eigenBuild --config Release
if ($LASTEXITCODE -ne 0) {
    throw "Eigen installation failed with exit code $LASTEXITCODE."
}

$eigenInclude = Join-Path $eigenInstall "include\eigen3"
if (-not (Test-Path -LiteralPath (Join-Path $eigenInclude "Eigen"))) {
    throw "Eigen installation did not produce the expected include directory: $eigenInclude"
}
$eigenIncludeCMake = $eigenInclude.Replace("\", "/")

& $environment.CMake -S $environment.Chrono -B $chronoBuild -G "Visual Studio 17 2022" -A x64 `
    "-DCMAKE_CONFIGURATION_TYPES=$Configuration" `
    "-DCMAKE_INSTALL_PREFIX=$chronoInstall" `
    "-DEIGEN3_INCLUDE_DIR=$eigenIncludeCMake" `
    -DBUILD_SHARED_LIBS=OFF `
    -DBUILD_DEMOS=OFF `
    -DBUILD_TESTING=OFF `
    -DBUILD_BENCHMARKING=OFF `
    -DCH_ENABLE_MODULE_VEHICLE=ON `
    -DCH_ENABLE_MODULE_VEHICLE_MODELS=ON `
    -DCH_ENABLE_MODULE_VEHICLE_COSIM=OFF `
    -DCH_ENABLE_OPENCRG=OFF `
    -DCH_ENABLE_OPENMP=OFF `
    -DCH_USE_MSVC_STATIC_RUNTIME=OFF
if ($LASTEXITCODE -ne 0) {
    throw "Chrono configuration failed with exit code $LASTEXITCODE."
}

Copy-Item -LiteralPath (Join-Path $environment.RepoRoot "cmake\Directory.Build.targets") `
    -Destination (Join-Path $chronoBuild "Directory.Build.targets") -Force
$disableCompilerBatching = if ($MaxCompilerJobs -eq 1) { "true" } else { "false" }

& $environment.CMake --build $chronoBuild --config $Configuration --target Chrono_core Chrono_vehicle ChronoModels_robot ChronoModels_vehicle -- `
    "/m:$MaxParallelJobs" "/p:CL_MPCount=$MaxCompilerJobs" "/p:LDGMDisableCompilerBatching=$disableCompilerBatching"
if ($LASTEXITCODE -ne 0) {
    throw "Chrono build failed with exit code $LASTEXITCODE."
}

& $environment.CMake --install $chronoBuild --config $Configuration
if ($LASTEXITCODE -ne 0) {
    throw "Chrono installation failed with exit code $LASTEXITCODE."
}

Write-Host "Built and installed Chrono Core/Vehicle ($Configuration) to $chronoInstall."
