<#
.SYNOPSIS
  Builds Dozen (Mesa's Vulkan-on-D3D12 driver) from external/mesa-uwp for UWP/Xbox.

.DESCRIPTION
  Output: build/mesa/install/bin/vulkan_dzn.dll, which the app packages and loads with
  LoadPackagedLibrary. Run from a "x64 Native Tools Command Prompt for VS 2022" (or after
  Enter-VsDevShell) so cl.exe and the Windows SDK are on PATH.

  Needs: Python 3 with meson and mako (pip install meson mako), ninja, win_flex/win_bison or
  flex/bison (winflexbison), and a DirectX Shader Compiler dxil.dll to ship next to the driver.

  The meson options come from the fork: -Duwp=true builds its UWP glue (src/util/uwp_util.cpp,
  CoreWindow/composition WSI), and _XBOX_UWP selects the Xbox code paths in the driver and WSI.
  If the fork changes how it is configured, follow its README and keep the output path.
#>
param(
	[ValidateSet('release', 'debugoptimized', 'debug')]
	[string]$BuildType = 'release'
)

$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $PSScriptRoot
$Mesa = Join-Path $Root 'external/mesa-uwp'
$Build = Join-Path $Root 'build/mesa'
$Install = Join-Path $Build 'install'

if (-not (Test-Path $Mesa)) { throw 'external/mesa-uwp is missing. Run scripts/bootstrap.ps1 first.' }
if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) { throw 'cl.exe not on PATH. Use a VS 2022 x64 developer prompt.' }

# UWP CRT and API partition. /D_XBOX_UWP turns on the fork's Xbox paths.
$uwpArgs = "['/D_XBOX_UWP', '/DWINAPI_FAMILY=WINAPI_FAMILY_APP', '/D_WIN32_WINNT=0x0A00']"

& meson setup $Build $Mesa --wipe `
	--buildtype=$BuildType `
	--prefix=$Install `
	--backend=ninja `
	-Duwp=true `
	-Dvulkan-drivers=microsoft-experimental `
	-Dgallium-drivers=[] `
	-Dopengl=false `
	-Dgles1=disabled -Dgles2=disabled -Degl=disabled -Dgbm=disabled -Dglx=disabled `
	-Dplatforms=windows `
	-Dllvm=disabled `
	-Dshared-glapi=disabled `
	-Dmicrosoft-clc=disabled `
	-Dspirv-to-dxil=false `
	-Db_vscrt=md `
	"-Dc_args=$uwpArgs" `
	"-Dcpp_args=$uwpArgs"
if ($LASTEXITCODE -ne 0) { throw 'meson setup failed' }

& meson install -C $Build
if ($LASTEXITCODE -ne 0) { throw 'meson install failed' }

$dll = Get-ChildItem -Recurse $Install -Filter 'vulkan_dzn.dll' | Select-Object -First 1
if (-not $dll) { throw "vulkan_dzn.dll not found under $Install" }

Copy-Item $dll.FullName (Join-Path $Build 'vulkan_dzn.dll') -Force
Write-Host "Dozen: $(Join-Path $Build 'vulkan_dzn.dll')" -ForegroundColor Green
Write-Host 'Remember to package dxil.dll (DirectX Shader Compiler) next to it if your Dozen build validates DXIL.'
