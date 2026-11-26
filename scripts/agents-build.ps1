#Requires -Version 5.1
<#
.SYNOPSIS
  Build the Auto Core production solution in Release x64.
.DESCRIPTION
  Builds app\AutoCore.sln Release x64 with devenv.com. That solution is
  production projects only. Test projects are built from app\Tests.sln.
.EXAMPLE
  .\scripts\agents-build.ps1
#>
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$RepoRoot = Split-Path -Parent $PSScriptRoot

$DevenvCommand = Get-Command devenv.com -ErrorAction SilentlyContinue
if (-not $DevenvCommand) {
    throw 'devenv.com not found on PATH. Install Visual Studio and open a shell where devenv.com is available.'
}
$Devenv = $DevenvCommand.Source

$Solutions = @(
    'app\AutoCore.sln'
)

foreach ($solution in $Solutions) {
    $path = Join-Path $RepoRoot $solution
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Missing solution: $path"
    }
    Write-Host "Building $solution"
    & $Devenv $path /Build 'Release|x64'
    if ($LASTEXITCODE -ne 0) {
        throw "devenv failed for $solution (exit $LASTEXITCODE)"
    }
}

Write-Host 'Release x64 build finished.'
exit 0
