<#
.SYNOPSIS
    Configure, compile et teste la formation depuis un PowerShell ordinaire.

.DESCRIPTION
    Ce script localise Visual Studio via vswhere, entre dans l'environnement
    developpeur (pour que cl.exe soit accessible), ajoute le CMake et le Ninja
    livres avec Visual Studio au PATH, puis enchaine configure / build / test.

    Pourquoi un script plutot que "cliquer dans l'IDE" ? Parce que la DO-178C
    exige un processus de production du code REPRODUCTIBLE et documente
    (section 7 : gestion de configuration). Tout ce qui n'est pas scripte
    finit par diverger d'un poste a l'autre.

.EXAMPLE
    .\scripts\build.ps1
    .\scripts\build.ps1 -Preset strict -Test
    .\scripts\build.ps1 -Clean
#>
[CmdletBinding()]
param(
    [ValidateSet('debug', 'release', 'strict', 'vs2026')]
    [string]$Preset = 'debug',

    [switch]$Test,

    [switch]$Clean
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot

# --- 1. Localiser Visual Studio ----------------------------------------------
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $vswhere)) {
    throw "vswhere.exe introuvable. Visual Studio est-il installe ?"
}

$vsPath = & $vswhere -latest -products * `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -property installationPath
if (-not $vsPath) {
    throw "Aucune installation Visual Studio avec les outils C++ (composant 'Desktop development with C++')."
}
Write-Host "Visual Studio : $vsPath" -ForegroundColor Cyan

# --- 2. Entrer dans l'environnement developpeur ------------------------------
# VsDevCmd.bat rappelle vswhere : on met son repertoire dans le PATH pour
# eviter un message d'erreur parasite.
$env:PATH = "$(Split-Path -Parent $vswhere);$env:PATH"

$devShell = Join-Path $vsPath 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll'
Import-Module $devShell
Enter-VsDevShell -VsInstallPath $vsPath -SkipAutomaticLocation `
    -DevCmdArguments '-arch=x64 -host_arch=x64' | Out-Null

# --- 3. Rendre CMake et Ninja accessibles ------------------------------------
$cmakeBin = Join-Path $vsPath 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin'
$ninjaBin = Join-Path $vsPath 'Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja'
$llvmBin  = Join-Path $vsPath 'VC\Tools\Llvm\x64\bin'   # clang-tidy, clang-format
foreach ($p in @($cmakeBin, $ninjaBin, $llvmBin)) {
    if ((Test-Path $p) -and ($env:PATH -notlike "*$p*")) {
        $env:PATH = "$p;$env:PATH"
    }
}

Set-Location $repoRoot

if ($Clean) {
    $buildDir = Join-Path $repoRoot 'build'
    if (Test-Path $buildDir) {
        Write-Host "Suppression de $buildDir" -ForegroundColor Yellow
        Remove-Item -Recurse -Force $buildDir
    }
}

# --- 4. Configure / Build / Test ---------------------------------------------
Write-Host "`n=== Configuration ($Preset) ===" -ForegroundColor Green
cmake --preset $Preset
if ($LASTEXITCODE -ne 0) { throw "Echec de la configuration CMake." }

Write-Host "`n=== Compilation ($Preset) ===" -ForegroundColor Green
cmake --build --preset $Preset
if ($LASTEXITCODE -ne 0) { throw "Echec de la compilation." }

if ($Test) {
    Write-Host "`n=== Tests ($Preset) ===" -ForegroundColor Green
    ctest --preset $Preset
    if ($LASTEXITCODE -ne 0) { throw "Des tests ont echoue." }
}

Write-Host "`nTermine." -ForegroundColor Green
