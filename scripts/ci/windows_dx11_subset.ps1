# Vendor-free DirectX 11 WARP smoke subset for PR CI.
$ErrorActionPreference = "Stop"
$Root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$Deps = Join-Path $env:RUNNER_TEMP "carbon-deps"
$Prefix = Join-Path $env:RUNNER_TEMP "carbon-prefix"
$SuperBuild = Join-Path $env:RUNNER_TEMP "dx11-build"

New-Item -ItemType Directory -Force -Path $Deps, $Prefix, $SuperBuild | Out-Null

$coreSrc = $env:CARBON_CORE_PATH
$imageioSrc = $env:CARBON_IMAGEIO_PATH
if (-not $coreSrc) { throw "CARBON_CORE_PATH required" }
if (-not $imageioSrc) { throw "CARBON_IMAGEIO_PATH required" }

cmake -S $coreSrc -B (Join-Path $Deps "core-build") -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_INSTALL_PREFIX=$Prefix `
  -DCMAKE_CXX_STANDARD=17 `
  -DWITH_TELEMETRY=OFF `
  -DBUILD_TESTING=OFF `
  -DBUILD_DOCUMENTATION=OFF `
  -DVCPKG_MANIFEST_MODE=OFF
cmake --build (Join-Path $Deps "core-build") --target install CoreConversionSmoke

$coreSmoke = Join-Path $Deps "core-build\CoreConversionSmoke.exe"
& $coreSmoke | Tee-Object -Variable coreOut
if ($LASTEXITCODE -ne 0 -or ($coreOut -notmatch "CORE_CONVERSION_SMOKE_OK")) { throw "CoreConversionSmoke failed" }

cmake -S $imageioSrc -B (Join-Path $Deps "imageio-build") -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_INSTALL_PREFIX=(Join-Path $Prefix "imageio") `
  -DCMAKE_PREFIX_PATH=$Prefix `
  -DCMAKE_CXX_STANDARD=17 `
  -DCARBON_IMAGEIO_WITH_NANOVDB=OFF `
  -DBUILD_TESTING=OFF
cmake --build (Join-Path $Deps "imageio-build") --target install

cmake -S (Join-Path $Root "ci\super-dx11-subset") -B $SuperBuild -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_PREFIX_PATH=$Prefix `
  -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL `
  -DCARBON_TRINITY_SOURCE_DIR=$Root `
  -DCARBON_IMAGEIO_SOURCE_DIR=$imageioSrc `
  -DCARBON_IMAGEIO_WITH_NANOVDB=OFF

$targets = @(
  "TrinityALDx11WarpSmoke",
  "TrinityALDx11WarpTriangleSmoke",
  "TrinityALAtlCompatSmoke",
  "TrinityALAdoptSelfAliasNegCtl"
)
foreach ($t in $targets) {
  cmake --build $SuperBuild --target $t
}

$env:PATH = "$Prefix\bin;$env:PATH"
$bin = Join-Path $SuperBuild "trinity\trinityal"
foreach ($exe in @("TrinityALDx11WarpSmoke.exe","TrinityALDx11WarpTriangleSmoke.exe","TrinityALAtlCompatSmoke.exe","TrinityALAdoptSelfAliasNegCtl.exe")) {
  $p = Join-Path $bin $exe
  Write-Host "=== $exe ==="
  & $p
  if ($LASTEXITCODE -ne 0) { throw "$exe exit $LASTEXITCODE" }
}
Write-Host "WINDOWS_DX11_SUBSET_CI_OK"