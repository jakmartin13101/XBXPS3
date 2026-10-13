<#
.SYNOPSIS
  Builds XBXPS3: the RPCS3 core (stage 1), Dozen, and the UWP app package (stage 2).

.EXAMPLE
  ./scripts/bootstrap.ps1
  ./scripts/build.ps1
  # -> build/app/AppPackages/XBXPS3_*/XBXPS3_*_x64.msix plus Dependencies/x64/*.appx

.NOTES
  Requires Visual Studio 2022 with "Desktop development with C++" and "Universal Windows
  Platform development" (C++ (v143) Universal Windows Platform tools), Windows SDK
  10.0.22621 or newer, CMake 3.28+, Ninja, Python 3 (with meson and mako for Dozen), Git.
  The first core build also compiles LLVM, which takes about an hour.
#>
param(
	[ValidateSet('all', 'core', 'mesa', 'app')]
	[string]$Stage = 'all',
	[switch]$Clean,
	[string]$SdkVersion = '10.0.22621.0'
)

$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root 'build'
$CoreBuild = Join-Path $BuildDir 'core'
$AppBuild = Join-Path $BuildDir 'app'
$CorePath = Join-Path $CoreBuild 'xbxps3_core'
$DozenDll = Join-Path $BuildDir 'mesa/vulkan_dzn.dll'
$Pfx = Join-Path $BuildDir 'XBXPS3_TemporaryKey.pfx'
$Cer = Join-Path $BuildDir 'XBXPS3.cer'

function Invoke-Checked {
	& $args[0] @($args | Select-Object -Skip 1)
	if ($LASTEXITCODE -ne 0) { throw "$($args[0]) failed ($LASTEXITCODE)" }
}

function Enter-VsEnvironment {
	if (Get-Command cl.exe -ErrorAction SilentlyContinue) { return }

	$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
	$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
	if (-not $vs) { throw 'Visual Studio 2022 with C++ tools not found' }

	Import-Module (Join-Path $vs 'Common7/Tools/Microsoft.VisualStudio.DevShell.dll')
	Enter-VsDevShell -VsInstallPath $vs -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64' | Out-Null
}

function Build-Core {
	if ($Clean -and (Test-Path $CoreBuild)) { Remove-Item -Recurse -Force $CoreBuild }

	# Series X|S are Zen 2: AVX2 is safe (XBSX2 ships its Series build the same way).
	# Xbox One consoles lack AVX2 and are not supported.
	Invoke-Checked cmake -S (Join-Path $Root 'external/rpcs3') -B $CoreBuild -G Ninja `
		-DCMAKE_BUILD_TYPE=Release `
		"-DCMAKE_CXX_FLAGS=/DWIN32 /D_WINDOWS /EHsc /arch:AVX2" `
		"-DCMAKE_C_FLAGS=/DWIN32 /D_WINDOWS /arch:AVX2" `
		-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL `
		-DRPCS3_UWP=ON `
		"-DXBXPS3_FRONTEND_DIR=$(Join-Path $Root 'core')" `
		-DBUILD_LLVM=ON `
		-DUSE_LTO=OFF `
		-DUSE_PRECOMPILED_HEADERS=ON `
		-DUSE_SYSTEM_ZLIB=OFF `
		-DUSE_SYSTEM_CURL=OFF `
		-DUSE_SYSTEM_OPENCV=OFF

	Invoke-Checked cmake --build $CoreBuild --target xbxps3_core
	Write-Host "Core: $(Join-Path $CorePath 'xbxps3_core.lib')" -ForegroundColor Green
}

function Get-SigningCertificate {
	if ((Test-Path $Pfx) -and (Test-Path $Cer)) { return }

	New-Item -ItemType Directory -Force $BuildDir | Out-Null

	# Subject must equal Publisher in app/Package.appxmanifest
	$cert = Get-ChildItem Cert:\CurrentUser\My | Where-Object { $_.Subject -eq 'CN=XBXPS3 Dev' } | Select-Object -First 1
	if (-not $cert) {
		$cert = New-SelfSignedCertificate -Type Custom -Subject 'CN=XBXPS3 Dev' -KeyUsage DigitalSignature `
			-FriendlyName 'XBXPS3 sideloading' -CertStoreLocation Cert:\CurrentUser\My `
			-TextExtension @('2.5.29.37={text}1.3.6.1.5.5.7.3.3', '2.5.29.19={text}')
	}

	# PFX for the build (password in app/CMakeLists.txt), .cer for Device Portal
	$password = ConvertTo-SecureString -String 'xbxps3' -Force -AsPlainText
	Export-PfxCertificate -Cert $cert -FilePath $Pfx -Password $password | Out-Null
	Export-Certificate -Cert $cert -FilePath $Cer | Out-Null
	Write-Host "Signing certificate: $Cer" -ForegroundColor Green
}

function Find-DxilDll {
	$sdkBin = Join-Path ${env:ProgramFiles(x86)} "Windows Kits/10/bin/$SdkVersion/x64/dxil.dll"
	if (Test-Path $sdkBin) { return $sdkBin }
	$any = Get-ChildItem (Join-Path ${env:ProgramFiles(x86)} 'Windows Kits/10/bin') -Recurse -Filter dxil.dll -ErrorAction SilentlyContinue |
		Where-Object { $_.FullName -match '\\x64\\' } | Sort-Object FullName -Descending | Select-Object -First 1
	if ($any) { return $any.FullName }
	return ''
}

function Build-App {
	if (-not (Test-Path (Join-Path $CorePath 'xbxps3_core.lib'))) { throw 'Core not built. Run with -Stage core first.' }
	if (-not (Test-Path $DozenDll)) { throw 'Dozen not built. Run with -Stage mesa first.' }
	if ($Clean -and (Test-Path $AppBuild)) { Remove-Item -Recurse -Force $AppBuild }

	Get-SigningCertificate
	$dxil = Find-DxilDll
	if (-not $dxil) { Write-Warning 'dxil.dll not found in the Windows SDK; Dozen may fail to create pipelines.' }

	Invoke-Checked cmake -S (Join-Path $Root 'app') -B $AppBuild -G 'Visual Studio 17 2022' -A x64 `
		-DCMAKE_SYSTEM_NAME=WindowsStore `
		"-DCMAKE_SYSTEM_VERSION=$SdkVersion" `
		"-DXBXPS3_CORE_DIR=$CorePath" `
		"-DXBXPS3_DOZEN_DLL=$DozenDll" `
		"-DXBXPS3_DXIL_DLL=$dxil" `
		"-DXBXPS3_CERT_PFX=$Pfx"

	Invoke-Checked cmake --build $AppBuild --config Release

	$msix = Get-ChildItem (Join-Path $AppBuild 'AppPackages') -Recurse -Include '*.msix', '*.appx' |
		Where-Object { $_.FullName -notmatch '\\Dependencies\\' } | Sort-Object LastWriteTime -Descending | Select-Object -First 1
	Write-Host "Package: $($msix.FullName)" -ForegroundColor Green
	Write-Host 'Deploy with: ./scripts/deploy.ps1 -Console <xbox-ip>'
}

Enter-VsEnvironment

switch ($Stage) {
	'core' { Build-Core }
	'mesa' { & (Join-Path $PSScriptRoot 'build-mesa.ps1') }
	'app'  { Build-App }
	'all'  {
		Build-Core
		if (-not (Test-Path $DozenDll)) { & (Join-Path $PSScriptRoot 'build-mesa.ps1') }
		Build-App
	}
}
