#Requires -Version 5.1
<#
.SYNOPSIS
  Build Auto Core Release x64 in the documented solution order.
.DESCRIPTION
  Builds app\AutoCore.sln Release x64 with devenv.com. That solution build
  is the shipped DLL and executables. Test projects are members and are not
  built. Renames locked dist\ outputs so Link can replace them.
.EXAMPLE
  .\scripts\build-all.ps1
#>
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$RepoRoot = Split-Path -Parent $PSScriptRoot

$DevenvCommand = Get-Command devenv.com -ErrorAction SilentlyContinue
if (-not $DevenvCommand) {
    throw 'devenv.com not found on PATH. Install Visual Studio and open a shell where devenv.com is available.'
}
$Devenv = $DevenvCommand.Source

function Test-FileWritable {
    param([Parameter(Mandatory)][string]$Path)
    try {
        $stream = [System.IO.File]::Open($Path, 'Open', 'ReadWrite', 'None')
        $stream.Dispose()
        return $true
    }
    catch {
        return $false
    }
}

function Unlock-DistFile {
    param([Parameter(Mandatory)][string]$Path)
    if (-not (Test-Path -LiteralPath $Path)) {
        return
    }
    if (Test-FileWritable $Path) {
        return
    }

    $name = Split-Path -Leaf $Path
    $staleName = '{0}.old.{1}' -f $name, [guid]::NewGuid().ToString('N')
    Write-Host "Renaming locked $name so the linker can replace it."
    Rename-Item -LiteralPath $Path -NewName $staleName
    Remove-Item -LiteralPath (Join-Path (Split-Path -Parent $Path) $staleName) `
        -Force -ErrorAction SilentlyContinue
}

# close_program does not wait for children. Leftover dist processes can
# still map auto_core.dll and *_ac.exe; rename those outputs so Link can
# replace them.

$DistDir = Join-Path $RepoRoot 'dist'
if (Test-Path -LiteralPath $DistDir) {
    $BinDir = Join-Path $DistDir 'bin'
    if (Test-Path -LiteralPath $BinDir) {
        Get-ChildItem -LiteralPath $BinDir -File -ErrorAction SilentlyContinue |
            Where-Object { $_.Extension -in '.exe', '.dll' } |
            ForEach-Object { Unlock-DistFile $_.FullName }
    }
}

$SymbolsDir = Join-Path $RepoRoot 'symbols'
if (Test-Path -LiteralPath $SymbolsDir) {
    Get-ChildItem -LiteralPath $SymbolsDir -Filter '*.pdb' -File -ErrorAction SilentlyContinue |
        ForEach-Object { Unlock-DistFile $_.FullName }
}

$Solutions = @(
    'app\AutoCore.sln'
)

foreach ($solution in $Solutions) {
    $path = Join-Path $RepoRoot $solution
    if (-not (Test-Path $path)) {
        throw "Missing solution: $solution"
    }
    Write-Host "Building $solution"
    & $Devenv $path /Build 'Release|x64'
    if ($LASTEXITCODE -ne 0) {
        throw "devenv failed for $solution (exit $LASTEXITCODE)"
    }
}

Write-Host 'Release x64 build finished.'
