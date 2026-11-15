# Builds Scrabble3D in release mode and makes a folder that runs without Qt installed,
# then a zip of it, and an installer when Inno Setup 6 is installed.
#
#   powershell -ExecutionPolicy Bypass -File deploy\package-windows.ps1
#   powershell -ExecutionPolicy Bypass -File deploy\package-windows.ps1 -QtDir C:\Qt\6.10.3\mingw_64 -SkipBuild
#
# The result is in dist\ : Scrabble3D\ (portable folder), Scrabble3D-<version>-windows.zip
# and, with Inno Setup, Scrabble3D-<version>-setup.exe.

param(
    [string]$QtDir = "C:\Qt\6.10.3\mingw_64",
    [string]$MinGWDir = "C:\Qt\Tools\mingw1310_64",
    [string]$BuildDir = "",
    [switch]$SkipBuild,
    [int]$Jobs = 2
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
if ($BuildDir -eq "") { $BuildDir = Join-Path $root "build-release" }
$dist = Join-Path $root "dist"
$app = Join-Path $dist "Scrabble3D"

foreach ($tool in @("$QtDir\bin\qmake.exe", "$QtDir\bin\windeployqt.exe", "$MinGWDir\bin\mingw32-make.exe")) {
    if (-not (Test-Path $tool)) { throw "Not found: $tool (see -QtDir and -MinGWDir)" }
}
$env:PATH = "$QtDir\bin;$MinGWDir\bin;$env:PATH"

# version from the project file (VERSION = x.y.z), "dev" when there is none
$version = "dev"
$pro = Get-Content (Join-Path $root "QScrabble.pro") -Raw
if ($pro -match "(?m)^\s*VERSION\s*=\s*([0-9.]+)") { $version = $Matches[1] }

if (-not $SkipBuild) {
    New-Item -ItemType Directory -Force $BuildDir | Out-Null
    Push-Location $BuildDir
    try {
        & qmake (Join-Path $root "QScrabble.pro") "CONFIG+=release"
        if ($LASTEXITCODE -ne 0) { throw "qmake failed" }
        & mingw32-make "-j$Jobs" release
        if ($LASTEXITCODE -ne 0) { throw "build failed" }
    } finally { Pop-Location }
}

$exe = Join-Path $BuildDir "release\Scrabble3D.exe"
if (-not (Test-Path $exe)) { throw "Not found: $exe" }

if (Test-Path $app) { Remove-Item -Recurse -Force $app }
New-Item -ItemType Directory -Force $app | Out-Null
Copy-Item $exe $app

# Qt libraries, QML modules, plugins (platform, TLS for the online services, images) and the MinGW runtime
& windeployqt --release --qmldir $root --no-translations --compiler-runtime (Join-Path $app "Scrabble3D.exe")
if ($LASTEXITCODE -ne 0) { throw "windeployqt failed" }

foreach ($f in @("README.md", "LICENSE", "COPYING")) {
    $p = Join-Path $root $f
    if (Test-Path $p) { Copy-Item $p $app }
}

$zip = Join-Path $dist "Scrabble3D-$version-windows.zip"
if (Test-Path $zip) { Remove-Item -Force $zip }
Compress-Archive -Path $app -DestinationPath $zip
Write-Host "Portable folder: $app"
Write-Host "Zip:             $zip"

# installer, only when Inno Setup 6 is installed
$iscc = @("${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe", "$env:ProgramFiles\Inno Setup 6\ISCC.exe",
          "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe") | Where-Object { Test-Path $_ } | Select-Object -First 1
if ($iscc) {
    & $iscc "/DAppVersion=$version" "/DSourceDir=$app" "/DOutputDir=$dist" (Join-Path $PSScriptRoot "scrabble3d.iss")
    if ($LASTEXITCODE -ne 0) { throw "Inno Setup failed" }
    Write-Host "Installer:       $dist\Scrabble3D-$version-setup.exe"
} else {
    Write-Host "Inno Setup 6 is not installed: no setup.exe (the zip can be shared as it is)."
}
