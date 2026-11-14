#Requires -Version 5.1

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$Stopwatch = [System.Diagnostics.Stopwatch]::StartNew()

$RepoRoot = Split-Path -Parent $PSScriptRoot
$AppSource = Join-Path $RepoRoot 'bin\app'
$ComponentsSource = Join-Path $RepoRoot 'bin\components'
$Dist = Join-Path $RepoRoot 'dist'
$Destination = Join-Path $Dist 'app'
$ComponentsDestination = Join-Path $Dist 'components'
$AppStaging = Join-Path $Dist 'app.new'
$ComponentsStaging = Join-Path $Dist 'components.new'

function Test-ComponentExecutableName {
    param (
        [Parameter(Mandatory)]
        [string]$Name
    )

    return $Name.EndsWith('_ac.exe', [System.StringComparison]::OrdinalIgnoreCase)
}

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
    if (-not (Test-Path -LiteralPath $AppSource -PathType Container)) {
        throw "Source directory not found: $AppSource"
    }
    if (-not (Test-Path -LiteralPath $ComponentsSource -PathType Container)) {
        throw "Source directory not found: $ComponentsSource"
    }

    $AppFiles = @(
        Get-ChildItem -LiteralPath $AppSource -File |
            Where-Object { $_.Extension -in '.exe', '.dll' }
    )
    if ($AppFiles.Count -eq 0) {
        throw "No executables or DLLs found in $AppSource"
    }

    $ComponentFiles = @(
        Get-ChildItem -LiteralPath $ComponentsSource -File |
            Where-Object { Test-ComponentExecutableName $_.Name }
    )
    if ($ComponentFiles.Count -eq 0) {
        throw "No component executables found in $ComponentsSource"
    }
    $PublishedComponents = [System.Collections.Generic.HashSet[string]]::new(
        [System.StringComparer]::OrdinalIgnoreCase
    )
    foreach ($File in $ComponentFiles) {
        [void]$PublishedComponents.Add($File.Name)
    }

    New-Item -ItemType Directory -Path $Dist -Force | Out-Null

    $RunningProcesses = @(Get-RunningDistProcesses -Directory $Destination)
    $RunningProcesses += @(
        @(Get-RunningDistProcesses -Directory $ComponentsDestination) |
            Where-Object {
                $PublishedComponents.Contains([System.IO.Path]::GetFileName($_.Path))
            }
    )

    if ($RunningProcesses.Count -gt 0) {
        Write-Host "Cannot update dist\app or dist\components because Auto Core binaries are currently running."

        foreach ($Process in $RunningProcesses) {
            Write-Host ("  {0}.exe (PID {1})" -f $Process.Name, $Process.Id)
        }

        Write-Host "Close the listed processes and run the script again."
        exit 2
    }

    foreach ($Staging in @($AppStaging, $ComponentsStaging)) {
        if (Test-Path -LiteralPath $Staging) {
            Remove-Item -LiteralPath $Staging -Recurse -Force
        }
    }

    New-Item -ItemType Directory -Path $AppStaging | Out-Null
    New-Item -ItemType Directory -Path $ComponentsStaging | Out-Null

    foreach ($File in $AppFiles) {
        Copy-Item -LiteralPath $File.FullName -Destination (Join-Path $AppStaging $File.Name) -Force
    }
    foreach ($File in $ComponentFiles) {
        Copy-Item -LiteralPath $File.FullName -Destination (Join-Path $ComponentsStaging $File.Name) -Force
    }

    New-Item -ItemType Directory -Path $Destination -Force | Out-Null
    New-Item -ItemType Directory -Path $ComponentsDestination -Force | Out-Null

    # dist\app is generated output. Remove its executables and DLLs, including
    # component EXEs published by older builds, then move the new Main set in.
    # Other files in dist\app are left alone.
    Get-ChildItem -LiteralPath $Destination -File |
        Where-Object { $_.Extension -in '.exe', '.dll' } |
        ForEach-Object { Remove-Item -LiteralPath $_.FullName -Force }

    Get-ChildItem -LiteralPath $AppStaging -File |
        ForEach-Object {
            Move-Item -LiteralPath $_.FullName -Destination (Join-Path $Destination $_.Name) -Force
        }

    # dist\components receives this build's *_ac.exe files. A manually added
    # third-party *_ac.exe is not deleted. A retired first-party EXE remains
    # until it is removed by hand.
    Get-ChildItem -LiteralPath $ComponentsStaging -File |
        ForEach-Object {
            Move-Item -LiteralPath $_.FullName -Destination (Join-Path $ComponentsDestination $_.Name) -Force
        }

    Write-Host "Updated bin\app -> dist\app and bin\components -> dist\components"
    exit 0
}
catch {
    Write-Host ""
    Write-Host "Update failed."
    Write-Host $_.Exception.Message
    exit 1
}
finally {
    foreach ($Staging in @($AppStaging, $ComponentsStaging)) {
        if (Test-Path -LiteralPath $Staging) {
            Remove-Item -LiteralPath $Staging -Recurse -Force -ErrorAction SilentlyContinue
        }
    }

    $Stopwatch.Stop()
    Write-Host ("Elapsed: {0:N3} seconds" -f $Stopwatch.Elapsed.TotalSeconds)
}
