[CmdletBinding()]
param(
    [ValidateSet("debug", "profile", "release")]
    [string]$Configuration = "profile",
    [ValidateRange(1, 60)]
    [int]$ProbeSeconds = 20
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "common.ps1")
$environment = Enter-LDGMEnvironment

$launcherDirectory = Join-Path $environment.RepoRoot "build\windows\bin\$Configuration"

# The server and client run concurrently: the authoritative host publishes
# pose snapshots over loopback and the client must log that it received them.
$launchers = @(
    @{ Name = "LDGM.ServerLauncher.exe"; Log = "Server.log"
       Expected = @("Chrono Core/Vehicle lifecycle smoke passed", "role=authoritative",
                    "T0 transform trace", "role=player", "role=enemy",
                    "active=2", "step_wall_ms=", "dropped_s=0.000000") },
    @{ Name = "LDGM.GameLauncher.exe"; Log = "Game.log"
       Expected = @("Chrono Core/Vehicle lifecycle smoke passed", "role=client-linkage",
                    "T0 snapshot trace", "presentations=2", "T0 proxy trace",
                    "mesh_component=true", "T0 client terrain presentation",
                    "T0 cockpit camera created", "T0 camera trace",
                    "camera_component=true active=true", "T0 input inventory") }
)

$startedAt = Get-Date
$processes = @()
try {
    foreach ($launcher in $launchers) {
        $launcherPath = Join-Path $launcherDirectory $launcher.Name
        if (-not (Test-Path -LiteralPath $launcherPath)) {
            throw "Launcher is not built: $launcherPath"
        }

        $process = Start-Process -FilePath $launcherPath `
            -ArgumentList @("--rhi=null", "-noprompt") `
            -WorkingDirectory $environment.RepoRoot `
            -WindowStyle Hidden `
            -PassThru
        $processes += @{ Launcher = $launcher; Process = $process; Satisfied = $false }
    }

    $stopwatch = [Diagnostics.Stopwatch]::StartNew()
    while ($stopwatch.Elapsed.TotalSeconds -lt $ProbeSeconds) {
        foreach ($entry in $processes) {
            $entry.Process.Refresh()
            if ($entry.Process.HasExited) {
                throw "$($entry.Launcher.Name) exited during the bounded probe with code $($entry.Process.ExitCode)."
            }

            if (-not $entry.Satisfied) {
                $logPath = Join-Path $environment.RepoRoot "user\log\$($entry.Launcher.Log)"
                if (Test-Path -LiteralPath $logPath) {
                    $log = Get-Item -LiteralPath $logPath
                    if ($log.LastWriteTime -ge $startedAt) {
                        $allObserved = $true
                        foreach ($expected in $entry.Launcher.Expected) {
                            if ($allObserved) {
                                $allObserved = Select-String -LiteralPath $logPath `
                                    -SimpleMatch $expected `
                                    -Quiet
                            }
                        }
                        $entry.Satisfied = $allObserved
                    }
                }
            }
        }

        Start-Sleep -Milliseconds 250
    }

    foreach ($entry in $processes) {
        $expectedList = $entry.Launcher.Expected -join "', '"
        if (-not $entry.Satisfied) {
            throw "$($entry.Launcher.Name) remained alive but did not log all expected markers: '$expectedList'."
        }
        Write-Host "$($entry.Launcher.Name) remained alive for $ProbeSeconds seconds and logged '$expectedList'."
    }
}
finally {
    foreach ($entry in $processes) {
        $entry.Process.Refresh()
        if (-not $entry.Process.HasExited) {
            Stop-Process -Id $entry.Process.Id
            $entry.Process.WaitForExit()
        }
    }
}

Write-Host "Client/server launcher smoke tests passed."
