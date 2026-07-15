[CmdletBinding()]
param(
    [ValidateSet("debug", "profile", "release")]
    [string]$Configuration = "profile",
    [ValidateRange(1, 60)]
    [int]$ProbeSeconds = 15
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "common.ps1")
$environment = Enter-LDGMEnvironment

$launcherDirectory = Join-Path $environment.RepoRoot "build\windows\bin\$Configuration"
$launchers = @(
    @{ Name = "LDGM.GameLauncher.exe"; Log = "Game.log"; Role = "role=client-linkage" },
    @{ Name = "LDGM.ServerLauncher.exe"; Log = "Server.log"; Role = "role=authoritative" }
)

foreach ($launcher in $launchers) {
    $launcherPath = Join-Path $launcherDirectory $launcher.Name
    if (-not (Test-Path -LiteralPath $launcherPath)) {
        throw "Launcher is not built: $launcherPath"
    }

    $logPath = Join-Path $environment.RepoRoot "user\log\$($launcher.Log)"
    $startedAt = Get-Date
    $process = Start-Process -FilePath $launcherPath `
        -ArgumentList @("--rhi=null", "-noprompt") `
        -WorkingDirectory $environment.RepoRoot `
        -WindowStyle Hidden `
        -PassThru

    $stopwatch = [Diagnostics.Stopwatch]::StartNew()
    $smokeObserved = $false
    try {
        while ($stopwatch.Elapsed.TotalSeconds -lt $ProbeSeconds) {
            $process.Refresh()
            if ($process.HasExited) {
                throw "$($launcher.Name) exited during the bounded probe with code $($process.ExitCode)."
            }

            if (-not $smokeObserved -and (Test-Path -LiteralPath $logPath)) {
                $log = Get-Item -LiteralPath $logPath
                if ($log.LastWriteTime -ge $startedAt) {
                    $lifecycleObserved = Select-String -LiteralPath $logPath `
                        -SimpleMatch "Chrono Core/Vehicle lifecycle smoke passed" `
                        -Quiet
                    $roleObserved = Select-String -LiteralPath $logPath `
                        -SimpleMatch $launcher.Role `
                        -Quiet
                    $smokeObserved = $lifecycleObserved -and $roleObserved
                }
            }

            Start-Sleep -Milliseconds 250
        }

        if (-not $smokeObserved) {
            throw "$($launcher.Name) remained alive but did not log the expected Chrono lifecycle role '$($launcher.Role)'."
        }

        Write-Host "$($launcher.Name) remained alive for $ProbeSeconds seconds and logged '$($launcher.Role)'."
    }
    finally {
        $process.Refresh()
        if (-not $process.HasExited) {
            Stop-Process -Id $process.Id
            $process.WaitForExit()
        }
    }
}

Write-Host "Client/server launcher smoke tests passed."
