#Requires -Version 5.1

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$ScriptNames = @(
    'stage-and-zip-auto-core.ps1',
    'stage-and-zip-symbols.ps1'
)

$Processes = @()

try {
    foreach ($Name in $ScriptNames) {
        $Path = Join-Path $PSScriptRoot $Name
        if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
            throw "Missing script: $Path"
        }

        $StartInfo = New-Object System.Diagnostics.ProcessStartInfo
        $StartInfo.FileName = 'powershell.exe'
        $StartInfo.Arguments = "-NoProfile -ExecutionPolicy Bypass -File `"$Path`""
        $StartInfo.UseShellExecute = $false
        $StartInfo.CreateNoWindow = $true

        $Process = New-Object System.Diagnostics.Process
        $Process.StartInfo = $StartInfo
        if (-not $Process.Start()) {
            throw "Failed to start script: $Path"
        }

        $Processes += [pscustomobject]@{
            Name     = $Name
            Process  = $Process
            ExitCode = $null
        }
    }

    foreach ($Entry in $Processes) {
        $Entry.Process.WaitForExit()
        $Entry.ExitCode = $Entry.Process.ExitCode
    }

    $Failures = @(
        foreach ($Entry in $Processes) {
            if ($Entry.ExitCode -ne 0) {
                "$($Entry.Name) failed (exit $($Entry.ExitCode))"
            }
        }
    )

    if ($Failures.Count -gt 0) {
        throw ($Failures -join [Environment]::NewLine)
    }

    Write-Host 'Staged and zipped Auto Core and symbols.'
}
catch {
    foreach ($Entry in $Processes) {
        if (-not $Entry.Process.HasExited) {
            $Entry.Process.WaitForExit()
        }
    }

    Write-Host ''
    Write-Host 'Stage and zip all failed.'
    Write-Host $_.Exception.Message
    exit 1
}
