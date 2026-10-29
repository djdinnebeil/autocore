#Requires -Version 5.1

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$RepoRoot = Split-Path -Parent $PSScriptRoot
$SymbolsRoot = Join-Path $RepoRoot 'release\latest\symbols'
$SymbolsZip = Join-Path $RepoRoot 'release\latest\AutoCoreSymbols.zip'

function New-DirectoryZip {
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

    $DestinationParent = Split-Path -Parent $Destination
    New-Item -ItemType Directory -Force -Path $DestinationParent | Out-Null

    if (Test-Path -LiteralPath $Destination) {
        Remove-Item -LiteralPath $Destination -Force
    }

    Compress-Archive -Path (Join-Path $Source '*') -DestinationPath $Destination
}

try {
    New-DirectoryZip -Source $SymbolsRoot -Destination $SymbolsZip

    Write-Host 'Created release\latest\AutoCoreSymbols.zip.'
}
catch {
    Write-Host ''
    Write-Host 'Zip failed.'
    Write-Host $_.Exception.Message
    exit 1
}
