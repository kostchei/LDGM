# LDGM Shareable Beta Launcher
# Launches the standalone client into NPC Combat Race Mode with pre-configured loopback

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = Split-Path -Parent $ScriptDir
$BuildDir = Join-Path $RepoRoot "build\windows\bin\Profile"
$GameLauncher = Join-Path $BuildDir "LDGM.GameLauncher.exe"
$ServerLauncher = Join-Path $BuildDir "LDGM.ServerLauncher.exe"

Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "  Legally Distinct Gorka Morka (LDGM) - Shareable Beta" -ForegroundColor Yellow
Write-Host "  Opening Game Mode: NPC Combat Race Circuit" -ForegroundColor Green
Write-Host "============================================================" -ForegroundColor Cyan

if (-not (Test-Path $GameLauncher)) {
    Write-Host "[!] GameLauncher binary not found at $GameLauncher" -ForegroundColor Red
    Write-Host "    Please run .\scripts\build-project.ps1 first to build the project." -ForegroundColor Yellow
    exit 1
}

Write-Host "[+] Launching Authoritative Physics Server..." -ForegroundColor Cyan
$serverProcess = Start-Process -FilePath $ServerLauncher -ArgumentList "--sv_isDedicated=1 --rhi=Null" -PassThru -NoNewWindow

Start-Sleep -Seconds 2

Write-Host "[+] Launching First-Person Combat Race Client..." -ForegroundColor Green
$clientProcess = Start-Process -FilePath $GameLauncher -ArgumentList "--cl_connect=127.0.0.1" -PassThru -NoNewWindow

Write-Host "[+] LDGM Beta Session active! Press CTRL+C to terminate." -ForegroundColor Green

try {
    $clientProcess.WaitForExit()
} finally {
    if (-not $serverProcess.HasExited) {
        Stop-Process -Id $serverProcess.Id -Force -ErrorAction SilentlyContinue
    }
}
