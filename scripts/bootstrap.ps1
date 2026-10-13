<#
.SYNOPSIS
  Fetches everything XBXPS3 builds from, at pinned versions, into external/.

.DESCRIPTION
  external/rpcs3      RPCS3 at the commit the patches were written against, with submodules,
                      and patches/rpcs3/*.patch applied on a local branch "xbxps3"
  external/volk       Vulkan meta-loader (matches the Vulkan SDK RPCS3 uses)
  external/mesa-uwp   Mesa fork with Dozen (Vulkan on D3D12) ported to UWP/Xbox
  external/fonts      DejaVu Sans saved as Arial.ttf, so the overlays have a font before
                      PS3 firmware (and its fonts) is installed

  Safe to re-run: existing checkouts are reused and only reset when -Force is given.
#>
param(
	[switch]$Force
)

$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $PSScriptRoot
$External = Join-Path $Root 'external'

# Pinned versions. Update the RPCS3 commit together with the patch series.
$Rpcs3Url     = 'https://github.com/RPCS3/rpcs3.git'
$Rpcs3Commit  = '2ba2f75bcea7ef6bdd687046c7fcfb790faf9853'
$VolkUrl      = 'https://github.com/zeux/volk.git'
$VolkTag      = 'vulkan-sdk-1.4.341.0'
$MesaUrl      = 'https://github.com/rodrigoandrigo/Mesa-UWP.git'
$MesaCommit   = '77ed30a769129d5365f610c0dd0a37e5c3d87920'
$DejaVuZipUrl = 'https://github.com/dejavu-fonts/dejavu-fonts/releases/download/version_2_37/dejavu-fonts-ttf-2.37.zip'

function Invoke-Git {
	& git @args
	if ($LASTEXITCODE -ne 0) { throw "git $($args -join ' ') failed ($LASTEXITCODE)" }
}

function Get-Checkout([string]$Url, [string]$Dir, [string]$Ref) {
	if ((Test-Path $Dir) -and $Force) {
		Remove-Item -Recurse -Force $Dir
	}
	if (-not (Test-Path $Dir)) {
		Invoke-Git clone --filter=blob:none $Url $Dir
	}
	Invoke-Git -C $Dir fetch --tags origin
	Invoke-Git -C $Dir -c advice.detachedHead=false checkout --force $Ref
}

New-Item -ItemType Directory -Force $External | Out-Null

Write-Host '== RPCS3' -ForegroundColor Cyan
$Rpcs3 = Join-Path $External 'rpcs3'
Get-Checkout $Rpcs3Url $Rpcs3 $Rpcs3Commit
Invoke-Git -C $Rpcs3 submodule sync --recursive
Invoke-Git -C $Rpcs3 submodule update --init --recursive --depth 1 --jobs 8

# Fresh branch from the pinned commit, then the Xbox patch series on top
Invoke-Git -C $Rpcs3 checkout --force -B xbxps3 $Rpcs3Commit
$patches = Get-ChildItem (Join-Path $Root 'patches/rpcs3') -Filter '*.patch' | Sort-Object Name
Invoke-Git -C $Rpcs3 -c core.autocrlf=false -c user.name=XBXPS3 -c user.email=xbxps3@localhost am --keep-cr @($patches.FullName)
Write-Host "Applied $($patches.Count) patches"

Write-Host '== volk' -ForegroundColor Cyan
Get-Checkout $VolkUrl (Join-Path $External 'volk') $VolkTag

Write-Host '== Mesa-UWP (Dozen)' -ForegroundColor Cyan
Get-Checkout $MesaUrl (Join-Path $External 'mesa-uwp') $MesaCommit

Write-Host '== Fonts' -ForegroundColor Cyan
$Fonts = Join-Path $External 'fonts'
if (-not (Test-Path (Join-Path $Fonts 'Arial.ttf')) -or $Force) {
	New-Item -ItemType Directory -Force $Fonts | Out-Null
	$zip = Join-Path $env:TEMP 'dejavu-fonts.zip'
	$unzip = Join-Path $env:TEMP 'dejavu-fonts'
	Invoke-WebRequest -Uri $DejaVuZipUrl -OutFile $zip
	Expand-Archive -Force $zip $unzip
	$ttf = Get-ChildItem -Recurse $unzip -Filter 'DejaVuSans.ttf' | Select-Object -First 1
	# RPCS3's Windows font list starts with Arial.ttf (Emu/RSX/Overlays/overlay_fonts.cpp)
	Copy-Item $ttf.FullName (Join-Path $Fonts 'Arial.ttf')
	$license = Get-ChildItem -Recurse $unzip -Filter 'LICENSE' | Select-Object -First 1
	if ($license) { Copy-Item $license.FullName (Join-Path $Fonts 'DejaVu-LICENSE.txt') }
}

Write-Host 'Bootstrap complete. Next: scripts/build.ps1' -ForegroundColor Green
