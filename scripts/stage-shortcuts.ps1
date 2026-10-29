#Requires -Version 5.1

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$RepoRoot = Split-Path -Parent $PSScriptRoot
$StageRoot = Join-Path $RepoRoot 'release\latest\stage'
$AppRoot = Join-Path $StageRoot 'app'

$Shortcuts = @(
    @{
        Target = Join-Path $AppRoot 'auto_core.exe'
        Link   = Join-Path $StageRoot 'Auto Core.lnk'
    },
    @{
        Target = Join-Path $AppRoot 'auto_core_settings.exe'
        Link   = Join-Path $StageRoot 'Auto Core Settings.lnk'
    }
)

function New-StageShortcut {
    param(
        [Parameter(Mandatory)][string]$Target,
        [Parameter(Mandatory)][string]$Link
    )

    if (-not (Test-Path -LiteralPath $Target -PathType Leaf)) {
        throw "Shortcut target not found: $Target"
    }

    $Shell = New-Object -ComObject WScript.Shell
    $Shortcut = $Shell.CreateShortcut($Link)
    $Shortcut.TargetPath = $Target
    $Shortcut.WorkingDirectory = Split-Path -Parent $Target
    $Shortcut.Save()
}

try {
    New-Item -ItemType Directory -Force -Path $StageRoot | Out-Null

    foreach ($Shortcut in $Shortcuts) {
        New-StageShortcut -Target $Shortcut.Target -Link $Shortcut.Link
    }

    Write-Host 'Staged shortcuts in release\latest\stage.'
}
catch {
    Write-Host ''
    Write-Host 'Shortcut stage failed.'
    Write-Host $_.Exception.Message
    exit 1
}
