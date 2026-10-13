<#
.SYNOPSIS
  Fails if XBXPS3.exe imports a DLL that does not exist on the Xbox OS.

.DESCRIPTION
  A UWP process on Xbox only gets the OneCore API sets plus a few redistributables. A
  single import of user32.dll, gdi32.dll, setupapi.dll, ... and the app dies before main()
  with no log. The core is compiled as desktop code, so this check runs after every link.

  When it fails: find which object pulls the symbol (link with /VERBOSE, or search the
  xbxps3_core.lib members with dumpbin /SYMBOLS), then either guard the call with RPCS3_UWP
  in patches/rpcs3, or add a stand-in to core/src/xbx_win32_shims.cpp.
#>
param(
	[Parameter(Mandatory)] [string]$Binary
)

$ErrorActionPreference = 'Stop'

# Prefixes that are fine to import on Xbox (UWP + Dev Mode)
$allowed = @(
	'api-ms-win-', 'ext-ms-win-', 'ntdll.dll', 'kernel32.dll', 'kernelbase.dll',
	'vcruntime140_app.dll', 'vcruntime140_1_app.dll', 'msvcp140_app.dll', 'msvcp140_1_app.dll', 'msvcp140_2_app.dll', 'concrt140_app.dll',
	'ucrtbase.dll', 'd3d12.dll', 'dxgi.dll', 'd3d11.dll', 'd2d1.dll', 'dwrite.dll', 'xaudio2_9.dll', 'xinputuap.dll',
	'ws2_32.dll', 'bcrypt.dll', 'crypt32.dll', 'combase.dll', 'ole32.dll', 'oleaut32.dll', 'rpcrt4.dll', 'mfplat.dll'
)

$dumpbin = Get-Command dumpbin.exe -ErrorAction SilentlyContinue
if (-not $dumpbin) {
	Write-Warning 'dumpbin.exe not on PATH; skipping import check'
	exit 0
}

$imports = & $dumpbin.Source /NOLOGO /DEPENDENTS $Binary |
	ForEach-Object { $_.Trim().ToLowerInvariant() } |
	Where-Object { $_ -match '\.dll$' }

$bad = $imports | Where-Object {
	$dll = $_
	-not ($allowed | Where-Object { $dll.StartsWith($_) })
}

Write-Host "Imports of $([IO.Path]::GetFileName($Binary)): $($imports -join ', ')"

if ($bad) {
	Write-Error "These DLLs are not available to UWP apps on Xbox: $($bad -join ', '). See scripts/check-imports.ps1 for how to fix it."
	exit 1
}

Write-Host 'Import check passed' -ForegroundColor Green
