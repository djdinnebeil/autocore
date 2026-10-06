#Requires -Version 5.1
<#
.SYNOPSIS
  Copy third-party vendor runtime DLLs into dist\bin\.
.DESCRIPTION
  Enumerates third_party/*/bin/*.dll and copies each DLL into dist\bin\.
  Creates dist\bin\ when it does not exist. Does not read or copy lib\.
  A destination already mapped by a leftover child is renamed aside first so
  the new file can replace it. Run this when committed vendor runtime DLLs
  need refreshing. Normal Auto Core builds do not invoke this script.
.EXAMPLE
  .\scripts\copy-vendor-dlls.ps1
#>
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$RepoRoot = Split-Path -Parent $PSScriptRoot
$DistBinDir = Join-Path $RepoRoot 'dist\bin'
$VendorRoot = Join-Path $RepoRoot 'third_party'

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
