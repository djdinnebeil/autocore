#Requires -Version 5.1

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$RepoRoot = Split-Path -Parent $PSScriptRoot
$Builder = Join-Path $RepoRoot 'release\latest\stage\app\keymap_builder.exe'

try {
    if (-not (Test-Path -LiteralPath $Builder -PathType Leaf)) {
        throw "keymap_builder.exe not found: $Builder"
    }

    & $Builder
    if ($LASTEXITCODE -ne 0) {
        throw "keymap_builder.exe failed (exit $LASTEXITCODE)"
    }

    Write-Host 'Staged keymap catalogs to release\latest\stage\keymap.'
}
catch {
    Write-Host ''
    Write-Host 'Keymap stage failed.'
    Write-Host $_.Exception.Message
    exit 1
}
