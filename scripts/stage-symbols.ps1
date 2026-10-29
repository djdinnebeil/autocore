#Requires -Version 5.1

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$RepoRoot = Split-Path -Parent $PSScriptRoot
$SymbolsSource = Join-Path $RepoRoot 'symbols'
$SymbolsDestination = Join-Path $RepoRoot 'release\latest\symbols'

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
    Copy-DirectoryContents -Source $SymbolsSource -Destination $SymbolsDestination

    Write-Host 'Staged symbols to release\latest\symbols.'
}
catch {
    Write-Host ''
    Write-Host 'Stage failed.'
    Write-Host $_.Exception.Message
    exit 1
}
