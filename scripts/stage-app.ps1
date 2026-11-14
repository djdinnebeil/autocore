#Requires -Version 5.1

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$RepoRoot = Split-Path -Parent $PSScriptRoot
$AppSource = Join-Path $RepoRoot 'bin\app'
$ComponentsSource = Join-Path $RepoRoot 'bin\components'
$StageRoot = Join-Path $RepoRoot 'release\latest\stage'
$AppDestination = Join-Path $StageRoot 'app'
$ComponentsDestination = Join-Path $StageRoot 'components'

function Copy-DirectoryContents {
    param(
        [Parameter(Mandatory)][string]$Source,
        [Parameter(Mandatory)][string]$Destination
    )

    if (-not (Test-Path -LiteralPath $Source -PathType Container)) {
        throw "Source directory not found: $Source"
    }

    $Entries = @(Get-ChildItem -LiteralPath $Source -Force)
    if ($Entries.Count -eq 0) {
        throw "Source directory is empty: $Source"
    }

    if (Test-Path -LiteralPath $Destination) {
        Remove-Item -LiteralPath $Destination -Recurse -Force
    }

    New-Item -ItemType Directory -Force -Path $Destination | Out-Null

    foreach ($Entry in $Entries) {
        Copy-Item -LiteralPath $Entry.FullName -Destination $Destination -Recurse -Force
    }
}

try {
    Copy-DirectoryContents -Source $AppSource -Destination $AppDestination
    Copy-DirectoryContents -Source $ComponentsSource -Destination $ComponentsDestination

    Write-Host 'Staged bin\app and bin\components to release\latest\stage.'
}
catch {
    Write-Host ''
    Write-Host 'Stage failed.'
    Write-Host $_.Exception.Message
    exit 1
}
