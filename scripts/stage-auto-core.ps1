#Requires -Version 5.1

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$Scripts = @(
    'stage-app.ps1',
    'stage-keymap.ps1',
    'stage-shortcuts.ps1',
    'stage-info.ps1'
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

    Write-Host 'Staged Auto Core to release\latest\stage.'
}
catch {
    Write-Host ''
    Write-Host 'Stage failed.'
    Write-Host $_.Exception.Message
    exit 1
}
