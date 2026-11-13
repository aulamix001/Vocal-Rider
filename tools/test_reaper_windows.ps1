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
$install = Start-Process -FilePath $installer -ArgumentList @('/S', "/D=$reaper") -Wait -PassThru
if ($install.ExitCode -ne 0) { throw "REAPER installer failed: $($install.ExitCode)" }
if (-not (Test-Path "$reaper/reaper.exe")) { throw 'REAPER executable missing.' }
# reaper.ini beside the executable selects portable mode (no user settings changed).
$vst2 = (Resolve-Path "$Build/VocalBalance_artefacts/Release/VST2").Path
"[REAPER]`nvstpath64=$vst2`n" | Set-Content "$reaper/reaper.ini" -Encoding ascii
New-Item -ItemType Directory -Path "$reaper/UserPlugins" -Force | Out-Null
Copy-Item "$Build/Release/reaper_vocalbalance_discovery.dll" "$reaper/UserPlugins/"
$canonical = Join-Path ${env:CommonProgramFiles} 'VST3'
New-Item -ItemType Directory -Path $canonical -Force | Out-Null
Copy-Item "$Build/VocalBalance_artefacts/Release/VST3/VocalBalance.vst3" $canonical -Recurse
$env:VOCAL_BALANCE_REAPER_REPORT = $Report
Write-Host 'Launching the dedicated REAPER scan test'
$process = Start-Process "$reaper/reaper.exe" -ArgumentList @('-new', '-nosplash') -PassThru
try {
  $deadline = (Get-Date).AddSeconds(150)
  Add-Type -AssemblyName UIAutomationClient
  Add-Type -AssemblyName UIAutomationTypes
  while (-not (Test-Path $Report) -and (Get-Date) -lt $deadline -and -not $process.HasExited) {
    Start-Sleep -Seconds 2
    # Dismiss only REAPER's evaluation reminder, without purchasing/activating.
    $condition = [System.Windows.Automation.PropertyCondition]::new([System.Windows.Automation.AutomationElement]::ProcessIdProperty, $process.Id)
    $elements = [System.Windows.Automation.AutomationElement]::RootElement.FindAll([System.Windows.Automation.TreeScope]::Descendants, $condition)
    foreach ($element in $elements) {
      if ($element.Current.Name -match '^(Still Evaluating|Evaluate)$' -and $element.Current.IsEnabled) {
        $pattern = $null
        if ($element.TryGetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern, [ref]$pattern)) { $pattern.Invoke() }
      }
    }
  }
  if (-not (Test-Path $Report)) {
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
