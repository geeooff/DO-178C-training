<#
.SYNOPSIS
    Mesure la couverture structurelle (instructions et branches) avec
    OpenCppCoverage.

.DESCRIPTION
    OpenCppCoverage est un outil gratuit et open source qui instrumente les
    binaires MSVC via les informations de debogage (.pdb). Il produit un
    rapport HTML ligne par ligne.

    INSTALLATION :
        winget install OpenCppCoverage.OpenCppCoverage
    ou telechargement depuis https://github.com/OpenCppCoverage/OpenCppCoverage

    LIMITE IMPORTANTE : OpenCppCoverage mesure la couverture d'INSTRUCTIONS.
    Il ne mesure NI la couverture de decision, NI le MC/DC. Pour ces deux
    criteres, il faut :
      * un outil commercial qualifie (VectorCAST, LDRA, Rational Test
        RealTime, Cantata) ;
      * ou une analyse manuelle appuyee sur des tables de decision, comme
        celle que produit mod11::analyze_mcdc.

    C'est une distinction a connaitre : beaucoup de projets croient couvrir
    le MC/DC avec un outil qui ne le mesure pas.

.EXAMPLE
    .\scripts\coverage.ps1
    .\scripts\coverage.ps1 -Preset debug -Module 11-couverture-structurelle
#>
[CmdletBinding()]
param(
    [string]$Preset = 'debug',
    [string]$Module = '',
    [string]$OutputDir = 'reports/coverage'
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $repoRoot

$exe = Get-Command OpenCppCoverage.exe -ErrorAction SilentlyContinue
if (-not $exe) {
    Write-Host "OpenCppCoverage introuvable." -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Installation :" -ForegroundColor Cyan
    Write-Host "    winget install OpenCppCoverage.OpenCppCoverage"
    Write-Host ""
    Write-Host "Ou : https://github.com/OpenCppCoverage/OpenCppCoverage/releases"
    exit 1
}

$binDir = Join-Path $repoRoot "build/$Preset/bin"
if (-not (Test-Path $binDir)) {
    throw "Repertoire $binDir introuvable. Lancez d'abord : .\scripts\build.ps1 -Preset $Preset"
}

$pattern = if ($Module) { "tests_$Module.exe" } else { 'tests_*.exe' }
$testExes = Get-ChildItem -Path $binDir -Filter $pattern
if ($testExes.Count -eq 0) {
    throw "Aucun executable de test correspondant a '$pattern' dans $binDir"
}

$fullOutput = Join-Path $repoRoot $OutputDir
New-Item -ItemType Directory -Force -Path $fullOutput | Out-Null

Write-Host "Mesure de la couverture sur $($testExes.Count) executable(s)..." -ForegroundColor Green

# --sources limite la mesure a NOTRE code : sans cela, le rapport inclut la
# bibliotheque standard, ce qui n'a aucun sens (elle n'est pas notre code
# source au sens DO-178C). Les repertoires de test sont exclus : on mesure la
# couverture du code de PRODUCTION, pas celle du harnais.
foreach ($test in $testExes) {
    $runArgs = @(
        '--export_type', "binary:$fullOutput\$($test.BaseName).cov"
        '--sources', "$repoRoot\modules"
        '--sources', "$repoRoot\common"
        '--excluded_sources', "$repoRoot\modules\*\tests"
        '--excluded_sources', "$repoRoot\common\tests"
        '--', $test.FullName
    )
    Write-Host "  -> $($test.Name)" -ForegroundColor DarkGray
    & OpenCppCoverage.exe @runArgs | Out-Null
}

# Fusion de tous les fichiers .cov en un rapport HTML unique.
$mergeArgs = @('--export_type', "html:$fullOutput")
foreach ($cov in Get-ChildItem -Path $fullOutput -Filter '*.cov') {
    $mergeArgs += @('--input_coverage', $cov.FullName)
}
& OpenCppCoverage.exe @mergeArgs | Out-Null

Write-Host ""
Write-Host "Rapport HTML : $fullOutput\index.html" -ForegroundColor Green
Write-Host ""
Write-Host "RAPPEL : cet outil mesure la couverture d'INSTRUCTIONS." -ForegroundColor Yellow
Write-Host "         Il ne mesure ni la couverture de decision, ni le MC/DC." -ForegroundColor Yellow
