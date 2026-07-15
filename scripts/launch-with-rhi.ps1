$Configuration = "profile"
$repoRoot = Split-Path -Parent $PSScriptRoot
$launcherDirectory = Join-Path $repoRoot "build\windows\bin\$Configuration"

$serverPath = Join-Path $launcherDirectory "LDGM.ServerLauncher.exe"
$clientPath = Join-Path $launcherDirectory "LDGM.GameLauncher.exe"

if (-not (Test-Path -LiteralPath $serverPath) -or -not (Test-Path -LiteralPath $clientPath)) {
    throw "Launchers not found in $launcherDirectory. Make sure the project is built."
}

Write-Host "Starting Authoritative Server (headless)..."
$serverProcess = Start-Process -FilePath $serverPath -ArgumentList "--rhi=null", "-noprompt" -WorkingDirectory $repoRoot -PassThru

Write-Host "Starting Game Client (with RHI rendering)..."
$clientProcess = Start-Process -FilePath $clientPath -ArgumentList "-noprompt" -WorkingDirectory $repoRoot -PassThru

Write-Host "Both processes started."
Write-Host "Press any key in this window to stop both launchers..."
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")

Write-Host "Stopping processes..."
if (-not $serverProcess.HasExited) { Stop-Process -Id $serverProcess.Id }
if (-not $clientProcess.HasExited) { Stop-Process -Id $clientProcess.Id }
Write-Host "Done."
