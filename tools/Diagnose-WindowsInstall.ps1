# Read-only unless -Unblock is explicitly supplied. Does not change Windows policies.
param(
  [string]$Module = 'C:\Program Files\Common Files\VST3\VocalBalance.vst3\Contents\x86_64-win\VocalBalance.vst3',
  [switch]$Unblock
)
$ErrorActionPreference = 'Stop'
$item = Get-Item -LiteralPath $Module
Get-FileHash -LiteralPath $item.FullName -Algorithm SHA256 | Format-List
Get-AuthenticodeSignature -LiteralPath $item.FullName | Select-Object Status,StatusMessage | Format-List
$zone = Get-Content -LiteralPath $item.FullName -Stream Zone.Identifier -ErrorAction SilentlyContinue
if ($zone) { Write-Output 'Mark of the Web:'; $zone } else { Write-Output 'No Zone.Identifier stream on this module.' }
if ($Unblock) { Unblock-File -LiteralPath $item.FullName; Write-Output 'Removed Zone.Identifier from this file only; Application Control policies are unchanged.' }
$bytes = [IO.File]::ReadAllBytes($item.FullName)
$offset = [BitConverter]::ToInt32($bytes,0x3c)
$machine = [BitConverter]::ToUInt16($bytes,$offset+4)
Write-Output ('PE machine: 0x{0:X4} (expected 8664 for x64)' -f $machine)
try {
  Get-WinEvent -FilterHashtable @{LogName='Microsoft-Windows-CodeIntegrity/Operational';StartTime=(Get-Date).AddDays(-2)} -MaxEvents 1000 -ErrorAction Stop |
    Where-Object { $_.Message -like '*VocalBalance*' -or $_.Message -like '*Vocal Balance*' } |
    Select-Object TimeCreated,Id,Message | Format-List
} catch { Write-Output "Code Integrity events unavailable or empty: $($_.Exception.Message)" }
$auditPath = Join-Path $PSScriptRoot 'windows-audit/0.2.1-audit.json'
if (Test-Path $auditPath) {
  $audit = Get-Content $auditPath -Raw | ConvertFrom-Json
  $format = if ($item.Extension -eq '.dll') { 'VST2' } else { 'VST3' }
  $expected = $audit.modules.$format
  Write-Output "Expected 0.2.1 SHA256: $($expected.sha256)"
  foreach ($dll in $expected.dependencies) {
    if ($dll -like 'api-ms-*' -or $dll -like 'ext-ms-*') {
      Write-Output "$dll : Windows API-set contract; absence of a physical file is not a missing-DLL diagnosis."
    } else {
      $present = Test-Path (Join-Path $env:SystemRoot "System32/$dll")
      Write-Output "$dll : present in System32 = $present"
    }
  }
}
