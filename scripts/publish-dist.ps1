#Requires -Version 5.1

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$Stopwatch = [System.Diagnostics.Stopwatch]::StartNew()

$RepoRoot = Split-Path -Parent $PSScriptRoot
$Source = Join-Path $RepoRoot 'bin'
$Dist = Join-Path $RepoRoot 'dist'
$Destination = Join-Path $Dist 'bin'
$Staging = Join-Path $Dist 'bin.new'

function Get-RunningDistProcesses {
    param (
        [Parameter(Mandatory)]
        [string]$Directory
    )

    if (-not (Test-Path -LiteralPath $Directory -PathType Container)) {
        return @()
    }

    $DirectoryPrefix = [System.IO.Path]::GetFullPath($Directory).TrimEnd('\') + '\'

    Get-Process | ForEach-Object {
        try {
            $Path = $_.Path

            if ($Path) {
                $FullPath = [System.IO.Path]::GetFullPath($Path)

                if ($FullPath.StartsWith(
                    $DirectoryPrefix,
                    [System.StringComparison]::OrdinalIgnoreCase
                )) {
                    [PSCustomObject]@{
                        Name = $_.ProcessName
                        Id   = $_.Id
                        Path = $FullPath
                    }
                }
            }
        }
        catch {
            # Some processes do not allow their executable path to be queried.
        }
    }
}

try {
    if (-not (Test-Path -LiteralPath $Source -PathType Container)) {
        throw "Source directory not found: $Source"
    }

    $RootSetupName = 'Auto Core Setup.exe'
    $SourceFiles = @(
        Get-ChildItem -LiteralPath $Source -File |
            Where-Object {
                $_.Extension -in '.exe', '.dll' -and $_.Name -ne $RootSetupName
            }
    )
    if ($SourceFiles.Count -eq 0) {
        throw "No executables or DLLs found in $Source"
    }

    New-Item -ItemType Directory -Path $Dist -Force | Out-Null

    $RunningProcesses = @(Get-RunningDistProcesses -Directory $Destination)

    if ($RunningProcesses.Count -gt 0) {
        Write-Host "Cannot publish to dist\bin because Auto Core binaries are currently running."

        foreach ($Process in $RunningProcesses) {
            Write-Host ("  {0}.exe (PID {1})" -f $Process.Name, $Process.Id)
        }

        Write-Host "Close the listed processes and run the script again."
        exit 2
    }

    if (Test-Path -LiteralPath $Staging) {
        Remove-Item -LiteralPath $Staging -Recurse -Force
    }

    New-Item -ItemType Directory -Path $Staging | Out-Null

    foreach ($File in $SourceFiles) {
        Copy-Item -LiteralPath $File.FullName -Destination (Join-Path $Staging $File.Name) -Force
    }

    New-Item -ItemType Directory -Path $Destination -Force | Out-Null

    Get-ChildItem -LiteralPath $Destination -File |
        Where-Object { $_.Extension -in '.exe', '.dll' } |
        ForEach-Object { Remove-Item -LiteralPath $_.FullName -Force }

    Get-ChildItem -LiteralPath $Staging -File |
        ForEach-Object {
            Move-Item -LiteralPath $_.FullName -Destination (Join-Path $Destination $_.Name) -Force
        }

    $RootSetupSource = Join-Path $Source $RootSetupName
    if (Test-Path -LiteralPath $RootSetupSource -PathType Leaf) {
        Copy-Item -LiteralPath $RootSetupSource -Destination (Join-Path $Dist $RootSetupName) -Force
        Write-Host "Published $RootSetupName -> dist\$RootSetupName"
    }

    Write-Host "Published bin -> dist\bin"
    exit 0
}
catch {
    Write-Host ""
    Write-Host "Publish failed."
    Write-Host $_.Exception.Message
    exit 1
}
finally {
    if (Test-Path -LiteralPath $Staging) {
        Remove-Item -LiteralPath $Staging -Recurse -Force -ErrorAction SilentlyContinue
    }

    $Stopwatch.Stop()
    Write-Host ("Elapsed: {0:N3} seconds" -f $Stopwatch.Elapsed.TotalSeconds)
}
