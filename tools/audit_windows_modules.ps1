param(
  [Parameter(Mandatory=$true)][string]$Artifacts,
  [Parameter(Mandatory=$true)][string]$Version,
  [string]$Output = 'docs/windows-audit',
  [switch]$RequireStatic
)
$ErrorActionPreference = 'Stop'
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$toolVersion = (Get-Content "$vs/VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt" -Raw).Trim()
$dumpbin = "$vs/VC/Tools/MSVC/$toolVersion/bin/Hostx64/x64/dumpbin.exe"
$mt = Get-ChildItem "${env:ProgramFiles(x86)}/Windows Kits/10/bin/*/x64/mt.exe" | Sort-Object FullName -Descending | Select-Object -First 1
if (-not (Test-Path $dumpbin) -or -not $mt) { throw 'Windows SDK inspection tools missing.' }
New-Item -ItemType Directory -Path $Output -Force | Out-Null
$modules = @{
  VST2 = Join-Path $Artifacts 'VST2/VocalBalance.dll'
  VST3 = Join-Path $Artifacts 'VST3/VocalBalance.vst3/Contents/x86_64-win/VocalBalance.vst3'
}
$summary = [ordered]@{ version=$Version; source=$env:GITHUB_SHA; static_crt_required=[bool]$RequireStatic; modules=@{} }
foreach ($format in @('VST2','VST3')) {
  $module = (Resolve-Path $modules[$format]).Path
  $prefix = Join-Path $Output "$Version-$format"
  $results = @{}
  foreach ($flag in @('headers','exports','dependents')) {
    $lines = @(& $dumpbin "/$flag" $module 2>&1)
    if ($LASTEXITCODE -ne 0) { throw "dumpbin /$flag failed: $module" }
    $lines | Set-Content "$prefix-$flag.txt" -Encoding utf8
    $results[$flag] = $lines -join "`n"
  }
  if ($results.headers -notmatch '(?im)^\s*8664 machine \(x64\)') { throw "Not PE x64: $module" }
  $required = if ($format -eq 'VST3') { @('GetPluginFactory','InitDll','ExitDll') } else { @('VSTPluginMain') }
  foreach ($name in $required) {
    if ($results.exports -notmatch "(?m)\s$([regex]::Escape($name))(?:\s|$)") { throw "Missing export $name in $module" }
  }
  $dependencies = @([regex]::Matches($results.dependents, '(?im)^\s+([\w.-]+\.dll)\s*$') | ForEach-Object { $_.Groups[1].Value } | Sort-Object -Unique)
  if ($dependencies.Count -eq 0) { throw 'No dependencies parsed; audit is incomplete.' }
  $crt = @($dependencies | Where-Object { $_ -match '^(MSVCP|VCRUNTIME|CONCRT|MSVCR|UCRTBASE|api-ms-win-crt-)' })
  if ($RequireStatic -and $crt.Count -gt 0) { throw "Unexpected dynamic CRT dependencies: $($crt -join ', ')" }
  # Extract manifest resource 2 only if present; mt's missing-resource result is recorded.
  $manifest = "$prefix-manifest.xml"
  $mtLog = @(& $mt.FullName -nologo "-inputresource:$module;#2" "-out:$manifest" 2>&1)
  $mtExit = $LASTEXITCODE
  $mtLog | Set-Content "$prefix-manifest-tool.txt" -Encoding utf8
  $hasManifest = $mtExit -eq 0 -and (Test-Path $manifest)
  if ($mtExit -ne 0 -and (($mtLog -join ' ') -notmatch 'c101008c|1812|1813|1814|resource')) { throw 'Unexpected manifest extraction failure.' }
  $crtManifest = $hasManifest -and ((Get-Content $manifest -Raw) -match 'Microsoft\.VC\d+\.(CRT|DebugCRT)')
  if ($RequireStatic -and $crtManifest) { throw 'Manifest binds to an external VC CRT assembly.' }
  $signature = Get-AuthenticodeSignature -LiteralPath $module
  $summary.modules[$format] = [ordered]@{
    sha256=(Get-FileHash -LiteralPath $module -Algorithm SHA256).Hash.ToLowerInvariant()
    machine='8664 (x64)'; exports=$required; dependencies=$dependencies; dynamic_crt=$crt
    manifest_resource_2_present=$hasManifest; manifest_external_crt=[bool]$crtManifest
    authenticode=$signature.Status.ToString()
  }
  Write-Host "$Version $format : x64; exports $($required -join ', '); CRT imports $($crt -join ', ')"
}
$summary | ConvertTo-Json -Depth 8 | Set-Content (Join-Path $Output "$Version-audit.json") -Encoding utf8
