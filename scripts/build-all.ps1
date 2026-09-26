#Requires -Version 5.1
<#
.SYNOPSIS
  Build Auto Core Release x64 in the documented MSBuild order.
.DESCRIPTION
  Locates an MSBuild installation supporting MSVC v145 / Visual Studio 2026
  (version 18+) with vswhere and builds every production solution. Core DLL
  first. Renames locked dist\ outputs so Link can replace them.
  There is no root .sln.
.EXAMPLE
  .\scripts\build-all.ps1
#>
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$RepoRoot = Split-Path -Parent $PSScriptRoot

function Find-MSBuild {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'

    if (-not (Test-Path $vswhere)) {
        throw 'vswhere.exe not found. Install Visual Studio 2026 (version 18+) or Build Tools with the C++ build tools (MSVC v145).'
    }

    $found = & $vswhere `
        -latest `
        -prerelease `
        -products * `
        -version '[18.0,)' `
        -requires Microsoft.Component.MSBuild `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        -find 'MSBuild\**\Bin\MSBuild.exe' |
        Select-Object -First 1

    if ($found) {
        return $found
    }

    throw 'Compatible MSBuild not found. Install Visual Studio 2026 (version 18+) or Build Tools with the C++ x64/x86 build tools (MSVC v145).'
}

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

$MSBuild = Find-MSBuild
$Common = @('/m', '/nologo', '/t:Build', '/p:Configuration=Release', '/p:Platform=x64')

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
    $SymbolsDir = Join-Path $DistDir 'symbols'
    if (Test-Path -LiteralPath $SymbolsDir) {
        Get-ChildItem -LiteralPath $SymbolsDir -Filter '*.pdb' -File -ErrorAction SilentlyContinue |
            ForEach-Object { Unlock-DistFile $_.FullName }
    }
}

$Solutions = @(
    'app\core\vs\auto_core_dll.sln'
    'app\main\runtime\vs\auto_core.sln'
    'app\main\config\auto_core\auto_core_config.sln'
    'app\main\config\components\components_config.sln'
    'app\main\editors\components\components_editor.sln'
    'app\main\config\keymap\keymap_config.sln'
    'app\main\editors\keymap\keymap_editor.sln'
    'app\main\config\shutdown\shutdown_config.sln'
    'app\main\config\crash_recovery\crash_recovery_config.sln'
    'app\components\logger\config\logger_config.sln'
    'app\components\dash\main\dash.sln'
    'app\components\dash\config\dash_config.sln'
    'app\components\itunes\main\itunes.sln'
    'app\components\itunes\config\itunes_config.sln'
    'app\components\itunes\star\itunes_star.sln'
    'app\components\journal\main\journal.sln'
    'app\components\journal\config\journal_config.sln'
    'app\components\logger\main\logger.sln'
    'app\components\server\main\server.sln'
    'app\components\server\config\server_config.sln'
    'app\components\slash\main\slash.sln'
    'app\components\slash\config\slash_config.sln'
    'app\components\spotify\main\spotify.sln'
    'app\components\spotify\config\spotify_config.sln'
    'app\components\spotify\oauth\spotify_oauth.sln'
    'app\components\spotify\star\spotify_star.sln'
    'app\components\taskbar\main\taskbar.sln'
    'app\components\taskbar\config\taskbar_config.sln'
    'app\components\wake\main\wake.sln'
    'app\components\wake\config\wake_config.sln'
    'app\components\writer\main\writer.sln'
    'app\components\writer\config\writer_config.sln'
)

foreach ($solution in $Solutions) {
    $path = Join-Path $RepoRoot $solution
    if (-not (Test-Path $path)) {
        throw "Missing solution: $solution"
    }
    Write-Host "Building $solution"
    & $MSBuild $path @Common
    if ($LASTEXITCODE -ne 0) {
        throw "MSBuild failed for $solution (exit $LASTEXITCODE)"
    }
}

Write-Host 'Release x64 build finished.'