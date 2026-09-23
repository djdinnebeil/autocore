#Requires -Version 5.1
<#
.SYNOPSIS
  Copy runtime DLLs into dist\bin\ (vendor bins plus auto_core.dll).
.DESCRIPTION
  Copies third_party/*/bin/*.dll into dist\bin\. Then copies auto_core.dll from
  lib\ into dist\bin\. Exits with an error if lib\auto_core.dll does not exist.
  A destination already mapped by a leftover child is renamed aside first so
  the new file can replace it.
.EXAMPLE
  .\scripts\copy-dist-dlls.ps1
#>
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$RepoRoot = Split-Path -Parent $PSScriptRoot
$DistDir = Join-Path $RepoRoot 'dist'
$DistBinDir = Join-Path $DistDir 'bin'
$VendorRoot = Join-Path $RepoRoot 'third_party'
$LibDll = Join-Path $RepoRoot 'lib\auto_core.dll'

function Copy-RuntimeDll {
    param(
        [Parameter(Mandatory)][string]$Source,
        [Parameter(Mandatory)][string]$Destination
    )

    if (-not (Test-Path -LiteralPath $Source)) {
        return
    }

    try {
        Copy-Item -LiteralPath $Source -Destination $Destination -Force
        return
    }
    catch {
        if (-not (Test-Path -LiteralPath $Destination)) {
            throw
        }
    }

    $destDir = Split-Path -Parent $Destination
    $staleName = '{0}.old.{1}' -f (Split-Path -Leaf $Destination), [guid]::NewGuid().ToString('N')
    Rename-Item -LiteralPath $Destination -NewName $staleName
    Copy-Item -LiteralPath $Source -Destination $Destination -Force
    Remove-Item -LiteralPath (Join-Path $destDir $staleName) -Force -ErrorAction SilentlyContinue
}

New-Item -ItemType Directory -Force -Path $DistBinDir | Out-Null

if (Test-Path -LiteralPath $VendorRoot) {
    Get-ChildItem -LiteralPath $VendorRoot -Directory | ForEach-Object {
        $VendorBinDir = Join-Path $_.FullName 'bin'
        if (-not (Test-Path -LiteralPath $VendorBinDir)) {
            return
        }
        Get-ChildItem -LiteralPath $VendorBinDir -File -ErrorAction SilentlyContinue |
            Where-Object { $_.Extension -eq '.dll' } |
            ForEach-Object {
                Copy-RuntimeDll -Source $_.FullName -Destination (Join-Path $DistBinDir $_.Name)
            }
    }
}

if (-not (Test-Path -LiteralPath $LibDll)) {
    throw 'auto_core.dll not found in lib. Build the core DLL or restore lib\auto_core.dll.'
}

Copy-RuntimeDll -Source $LibDll -Destination (Join-Path $DistBinDir 'auto_core.dll')
