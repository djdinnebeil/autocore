#Requires -Version 5.1

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$Scripts = @(
    'stage-auto-core.ps1',
    'zip-auto-core.ps1'
)

try {
    foreach ($Name in $Scripts) {
        $Path = Join-Path $PSScriptRoot $Name
        if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
            throw "Missing script: $Path"
        }

        & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $Path
        if ($LASTEXITCODE -ne 0) {
            throw "$Name failed (exit $LASTEXITCODE)"
        }
    }

    Write-Host 'Staged and zipped Auto Core.'
}
catch {
    Write-Host ''
    Write-Host 'Stage and zip failed.'
    Write-Host $_.Exception.Message
    exit 1
}
