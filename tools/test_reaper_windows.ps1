param([string]$Build = 'build-msvc', [Parameter(Mandatory=$true)][string]$Report)
$ErrorActionPreference = 'Stop'
# Fresh hosted runner only; use a dedicated portable installation and resource directory.
$reaper = Join-Path $env:RUNNER_TEMP 'VocalBalance-Reaper'
New-Item -ItemType Directory -Path $reaper -Force | Out-Null
Write-Host 'Downloading the official REAPER installer'
$page = Invoke-WebRequest 'https://www.reaper.fm/download.php' -UseBasicParsing
$match = [regex]::Match($page.Content, 'files/[0-9]+\.x/reaper[0-9]+_x64-install\.exe')
if (-not $match.Success) { throw 'Official REAPER x64 download was not found.' }
$url = 'https://www.reaper.fm/' + $match.Value
$installer = Join-Path $env:RUNNER_TEMP 'reaper-install.exe'
Invoke-WebRequest $url -OutFile $installer
Write-Host "Downloaded $url"
$signature = Get-AuthenticodeSignature -FilePath $installer
if ($signature.Status -ne 'Valid') { throw "REAPER installer signature invalid: $($signature.Status)" }
Write-Host "Installer signature: $($signature.Status)"
# Extract the verified installer into an isolated portable directory. NSIS
# installation requests elevation (1223 on the hosted desktop); extraction
# does not modify the registry or require the installer UI.
& 7z x $installer "-o$reaper" -y
if ($LASTEXITCODE -ne 0) { throw 'Could not extract the official REAPER installer.' }
$executable = Get-ChildItem -LiteralPath $reaper -Filter reaper.exe -Recurse | Select-Object -First 1
if (-not $executable) { throw 'REAPER executable missing after extraction.' }
$reaper = $executable.DirectoryName
Write-Host "Portable REAPER: $($executable.FullName)"
# reaper.ini beside the executable selects portable mode (no user settings changed).
$vst2 = (Resolve-Path "$Build/VocalBalance_artefacts/Release/VST2").Path
"[REAPER]`nvstpath64=$vst2`n" | Set-Content "$reaper/reaper.ini" -Encoding ascii
New-Item -ItemType Directory -Path "$reaper/UserPlugins" -Force | Out-Null
Copy-Item "$Build/Release/reaper_vocalbalance_discovery.dll" "$reaper/UserPlugins/"
$canonical = Join-Path ${env:CommonProgramFiles} 'VST3'
New-Item -ItemType Directory -Path $canonical -Force | Out-Null
Copy-Item "$Build/VocalBalance_artefacts/Release/VST3/VocalBalance.vst3" $canonical -Recurse
# Cover the standard resource path too on this disposable hosted runner.
$resource = Join-Path $env:APPDATA 'REAPER'
New-Item -ItemType Directory -Path "$resource/UserPlugins" -Force | Out-Null
Copy-Item "$Build/Release/reaper_vocalbalance_discovery.dll" "$resource/UserPlugins/"
Copy-Item "$reaper/reaper.ini" "$resource/reaper.ini"
$env:VOCAL_BALANCE_REAPER_REPORT = $Report
Write-Host 'Launching the dedicated REAPER scan test'
$process = Start-Process "$reaper/reaper.exe" -ArgumentList @('-new', '-nosplash', '-cfgfile', "$reaper/reaper.ini") -PassThru
try {
  $deadline = (Get-Date).AddSeconds(150)
  Add-Type @'
using System;
using System.Text;
using System.Collections.Generic;
using System.Runtime.InteropServices;
public static class ReaperWindows {
 public delegate bool EnumProc(IntPtr w,IntPtr p);
 [DllImport("user32.dll")] static extern bool EnumWindows(EnumProc cb,IntPtr p);
 [DllImport("user32.dll")] static extern bool EnumChildWindows(IntPtr w,EnumProc cb,IntPtr p);
 [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr w,out uint p);
 [DllImport("user32.dll",CharSet=CharSet.Unicode)] static extern int GetWindowText(IntPtr w,StringBuilder s,int n);
 [DllImport("user32.dll")] static extern IntPtr SendMessage(IntPtr w,uint m,IntPtr a,IntPtr b);
 public static string[] Inspect(uint pid) {
  var labels=new List<string>();
  EnumProc visit=(w,p)=>{var s=new StringBuilder(2048);GetWindowText(w,s,2048);string t=s.ToString();
   if(t.Length>0) labels.Add(t);
   if(t.IndexOf("Still Evaluating",StringComparison.OrdinalIgnoreCase)>=0 || t=="OK") SendMessage(w,0xF5,IntPtr.Zero,IntPtr.Zero);
   return true;};
  EnumWindows((w,p)=>{uint id;GetWindowThreadProcessId(w,out id);if(id==pid){visit(w,p);EnumChildWindows(w,visit,p);}return true;},IntPtr.Zero);
  return labels.ToArray();
 }
}
'@
  $labels = @()
  while (-not (Test-Path $Report) -and (Get-Date) -lt $deadline -and -not $process.HasExited) {
    Start-Sleep -Seconds 2
    $labels = [ReaperWindows]::Inspect($process.Id)
  }
  if (-not (Test-Path $Report)) {
    Write-Host ($labels -join " | ")
    if (Test-Path "$Report.loaded") { Get-Content "$Report.loaded" }
    $loaded = Test-Path "$Report.loaded"
    throw "REAPER did not produce its scan report. Extension loaded: $loaded; process exited: $($process.HasExited)"
  }
  $data = Get-Content $Report -Raw | ConvertFrom-Json
  Get-Content $Report
  if (-not $data.passed) { throw 'REAPER failed to discover and instantiate both formats.' }
} finally {
  if (-not $process.HasExited) { Stop-Process -Id $process.Id -Force }
  Get-ChildItem "$reaper/reaper-vstplugins*.ini" | ForEach-Object { Copy-Item $_.FullName "$env:RUNNER_TEMP/$($_.Name)" }
}
