#Requires -Version 5.1
<#
.SYNOPSIS
  Build Auto Core Release x64 in the documented solution order.
.DESCRIPTION
  Builds app\AutoCore.sln Release x64 with devenv.com. That solution is
  production projects only. Test projects are built from app\Tests.sln.
  After a successful build, publishes bin\ into dev\app.
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

# Call update-dist.ps1 to write to dist\app

$PublishScript = Join-Path $PSScriptRoot 'update-dist.ps1'
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File $PublishScript
$PublishCode = $LASTEXITCODE

if ($PublishCode -eq 0) {
    exit 0
}

if ($PublishCode -eq 2) {
    Write-Warning 'Build succeeded. dev\app was not updated because Auto Core is running. Run scripts\update-dist.ps1 after closing it.'
    exit 0
}

Write-Host "Build succeeded, but publishing dev\app failed (exit $PublishCode)."
exit $PublishCode
