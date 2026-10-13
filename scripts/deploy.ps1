<#
.SYNOPSIS
  Installs the XBXPS3 package on an Xbox in Developer Mode through Xbox Device Portal.

.DESCRIPTION
  Uses the Device Portal REST API (the same calls the "Add" button in the web UI makes):
    POST /api/app/packagemanager/package?package=<name>   multipart: package, dependencies, certificate
    GET  /api/app/packagemanager/state                    poll until the install finishes
  Device Portal must be enabled in Dev Home, with a user name and password set there.
  Needs PowerShell 7 (the console uses a self-signed TLS certificate).

  After the first install: Dev Home > XBXPS3 > View details > App type: Game.

.EXAMPLE
  ./scripts/deploy.ps1 -Console 192.168.1.50
#>
param(
	[Parameter(Mandatory)] [string]$Console,
	[string]$Package,
	[pscredential]$Credential = (Get-Credential -Message 'Xbox Device Portal user name and password (set in Dev Home)')
)

$ErrorActionPreference = 'Stop'

if ($PSVersionTable.PSVersion.Major -lt 7) { throw 'Use PowerShell 7 (pwsh) for -SkipCertificateCheck.' }

$Root = Split-Path -Parent $PSScriptRoot
$AppPackages = Join-Path $Root 'build/app/AppPackages'

if (-not $Package) {
	$Package = (Get-ChildItem $AppPackages -Recurse -Include '*.msix', '*.appx' |
		Where-Object { $_.FullName -notmatch '\\Dependencies\\' } |
		Sort-Object LastWriteTime -Descending | Select-Object -First 1).FullName
}
if (-not $Package -or -not (Test-Path $Package)) { throw 'No package found. Run scripts/build.ps1 first or pass -Package.' }

$packageDir = Split-Path -Parent $Package
$dependencies = Get-ChildItem (Join-Path $packageDir 'Dependencies/x64') -Filter '*.appx' -ErrorAction SilentlyContinue
$certificate = Join-Path $Root 'build/XBXPS3.cer'

$base = "https://${Console}:11443"
$session = New-Object Microsoft.PowerShell.Commands.WebRequestSession

# Device Portal wants the CSRF-Token cookie echoed back in a header on POST requests
Invoke-WebRequest -Uri "$base/api/os/info" -Credential $Credential -SkipCertificateCheck -WebSession $session | Out-Null
$csrf = ($session.Cookies.GetCookies($base) | Where-Object Name -eq 'CSRF-Token').Value
$headers = @{}
if ($csrf) { $headers['X-CSRF-Token'] = $csrf }

$form = @{ package = Get-Item $Package }
$i = 0
foreach ($dep in $dependencies) { $form["dependency$i"] = $dep; $i++ }
if (Test-Path $certificate) { $form['certificate'] = Get-Item $certificate }

$name = [IO.Path]::GetFileName($Package)
Write-Host "Uploading $name with $($dependencies.Count) dependencies to $Console..."
Invoke-WebRequest -Method Post -Uri "$base/api/app/packagemanager/package?package=$([uri]::EscapeDataString($name))" `
	-Form $form -Headers $headers -Credential $Credential -SkipCertificateCheck -WebSession $session | Out-Null

# 204 = still installing, 200 = done (body has the result)
while ($true) {
	Start-Sleep -Seconds 2
	$state = Invoke-WebRequest -Uri "$base/api/app/packagemanager/state" -Credential $Credential -SkipCertificateCheck -WebSession $session
	if ($state.StatusCode -eq 200) {
		$result = $state.Content | ConvertFrom-Json
		if ($result.Success) {
			Write-Host 'Installed. In Dev Home set XBXPS3 to "Game", then launch it.' -ForegroundColor Green
		} else {
			throw "Install failed: $($result.Reason) (code $($result.Code))"
		}
		break
	}
	Write-Host -NoNewline '.'
}
