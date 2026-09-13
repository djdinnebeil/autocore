#Requires -Version 5.1
<#
.SYNOPSIS
  Copy runtime DLLs into dist\ (vendor bins plus auto_core.dll).
.DESCRIPTION
  Copies third_party/*/bin/*.dll into dist\. Then copies auto_core.dll into
  dist\, preferring out\core\auto_core.dll over lib\auto_core.dll. Exits
  with an error if neither auto_core.dll exists.
.EXAMPLE
  .\scripts\copy-dist-dlls.ps1
#>
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$RepoRoot = Split-Path -Parent $PSScriptRoot
$DistDir = Join-Path $RepoRoot 'dist'
$VendorRoot = Join-Path $RepoRoot 'third_party'
$GeneratedDll = Join-Path $RepoRoot 'out\core\auto_core.dll'
$SeedDll = Join-Path $RepoRoot 'lib\auto_core.dll'

New-Item -ItemType Directory -Force -Path $DistDir | Out-Null

if (Test-Path -LiteralPath $VendorRoot) {
    Get-ChildItem -LiteralPath $VendorRoot -Directory | ForEach-Object {
        $binDir = Join-Path $_.FullName 'bin'
        if (-not (Test-Path -LiteralPath $binDir)) {
            return
        }
        Get-ChildItem -LiteralPath $binDir -Filter '*.dll' -File -ErrorAction SilentlyContinue |
            ForEach-Object {
                Copy-Item -LiteralPath $_.FullName -Destination (Join-Path $DistDir $_.Name) -Force
            }
    }
}

$sourceDll = $null
if (Test-Path -LiteralPath $GeneratedDll) {
    $sourceDll = $GeneratedDll
}
elseif (Test-Path -LiteralPath $SeedDll) {
    $sourceDll = $SeedDll
}
else {
    throw 'auto_core.dll not found in out\core or lib. Build the core DLL or restore lib\auto_core.dll.'
}

Copy-Item -LiteralPath $sourceDll -Destination (Join-Path $DistDir 'auto_core.dll') -Force
