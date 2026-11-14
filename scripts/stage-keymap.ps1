#Requires -Version 5.1

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$RepoRoot = Split-Path -Parent $PSScriptRoot
$Builder = Join-Path $RepoRoot 'release\latest\stage\app\auto_core_keymap.exe'

try {
    if (-not (Test-Path -LiteralPath $Builder -PathType Leaf)) {
        throw "auto_core_keymap.exe not found: $Builder"
    }

    & $Builder --build
    if ($LASTEXITCODE -ne 0) {
        throw "auto_core_keymap.exe --build failed (exit $LASTEXITCODE)"
    }

    Write-Host 'Staged keymap catalogs to release\latest\stage\keymap.'
}
catch {
    Write-Host ''
    Write-Host 'Keymap stage failed.'
    Write-Host $_.Exception.Message
    exit 1
}
