#Requires -Version 5.1
<#
.SYNOPSIS
  Build Auto Core Release x64 from the 12 family solutions.
.DESCRIPTION
  Builds Core.sln, Main.sln, and each component family solution sequentially
  with devenv.com /Build "Release|x64". Core first. Test projects are members
  of those solutions and are not built. Renames locked symbol files so Link
  can replace them. Does not publish dist\bin.
.EXAMPLE
  .\scripts\build-all-components.ps1
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

# A debugger can keep symbols\*.pdb mapped. Rename those so Link can replace them.
# Shipped binaries publish to bin\. This script does not update dist\bin.

$SymbolsDir = Join-Path $RepoRoot 'symbols'
if (Test-Path -LiteralPath $SymbolsDir) {
    Get-ChildItem -LiteralPath $SymbolsDir -Filter '*.pdb' -File -ErrorAction SilentlyContinue |
        ForEach-Object { Unlock-DistFile $_.FullName }
}

$Solutions = @(
    'app\core\Core.sln'
    'app\main\Main.sln'
    'app\components\dash\Dash.sln'
    'app\components\itunes\iTunes.sln'
    'app\components\journal\Journal.sln'
    'app\components\logger\Logger.sln'
    'app\components\server\Server.sln'
    'app\components\slash\Slash.sln'
    'app\components\spotify\Spotify.sln'
    'app\components\taskbar\Taskbar.sln'
    'app\components\wake\Wake.sln'
    'app\components\writer\Writer.sln'
)

foreach ($solution in $Solutions) {
    $path = Join-Path $RepoRoot $solution
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Missing solution: $solution"
    }
    Write-Host "Building $solution"
    & $Devenv $path /Build 'Release|x64'
    if ($LASTEXITCODE -ne 0) {
        throw "devenv failed for $solution (exit $LASTEXITCODE)"
    }
}

Write-Host 'Release x64 build finished.'
