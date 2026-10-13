#Requires -Version 5.1

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$RepoRoot = Split-Path -Parent $PSScriptRoot
$DistRoot = Join-Path $RepoRoot 'dist'

$LegalSource = @(
    (Join-Path $RepoRoot 'notice.md'),
    (Join-Path $RepoRoot 'license')
)

$ManualSource = Join-Path $RepoRoot 'docs\manual'
$TutorialSource = Join-Path $RepoRoot 'docs\tutorial'

$TopLevelDocs = @(
    'Auto Core Manual.md',
    'Auto Core Tutorial.md',
    'Auto Core Guide.md'
)

try {
    New-Item -ItemType Directory -Force -Path $DistRoot | Out-Null

    $LegalDestination = Join-Path $DistRoot 'legal'
    $ManualDestination = Join-Path $DistRoot 'manual'
    $TutorialDestination = Join-Path $DistRoot 'tutorial'

    foreach ($path in $LegalSource) {
        if (-not (Test-Path -LiteralPath $path)) {
            throw "Missing documentation source: $path"
        }
    }

    if (-not (Test-Path -LiteralPath $ManualSource)) {
        throw "Missing documentation source: $ManualSource"
    }

    if (-not (Test-Path -LiteralPath $TutorialSource)) {
        throw "Missing documentation source: $TutorialSource"
    }

    foreach ($name in $TopLevelDocs) {
        $path = Join-Path $RepoRoot "docs\$name"
        if (-not (Test-Path -LiteralPath $path)) {
            throw "Missing documentation source: $path"
        }
    }

    Remove-Item -LiteralPath $LegalDestination -Recurse -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath $ManualDestination -Recurse -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath $TutorialDestination -Recurse -Force -ErrorAction SilentlyContinue

    foreach ($name in $TopLevelDocs) {
        Remove-Item -LiteralPath (Join-Path $DistRoot $name) -Force -ErrorAction SilentlyContinue
    }

    New-Item -ItemType Directory -Force -Path $LegalDestination | Out-Null

    Copy-Item -LiteralPath (Join-Path $RepoRoot 'notice.md') `
        -Destination (Join-Path $LegalDestination 'notice.md')

    Copy-Item -LiteralPath (Join-Path $RepoRoot 'license') `
        -Destination (Join-Path $LegalDestination 'license')

    Copy-Item -LiteralPath $ManualSource `
        -Destination $ManualDestination `
        -Recurse

    Copy-Item -LiteralPath $TutorialSource `
        -Destination $TutorialDestination `
        -Recurse

    foreach ($name in $TopLevelDocs) {
        Copy-Item -LiteralPath (Join-Path $RepoRoot "docs\$name") `
            -Destination (Join-Path $DistRoot $name)
    }

    Write-Host 'Published documentation to dist.'
}
catch {
    Write-Host ''
    Write-Host 'Documentation publish failed.'
    Write-Host $_.Exception.Message
    exit 1
}