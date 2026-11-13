[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$CertificateThumbprint,
    [string]$ArtifactsDirectory = (Join-Path $PSScriptRoot '..\build\VocalBalance_artefacts\Release'),
    [string]$SignToolPath,
    [string]$TimestampServer = 'http://timestamp.digicert.com'
)
$ErrorActionPreference = 'Stop'
$thumbprint = ($CertificateThumbprint -replace '\s', '').ToUpperInvariant()
$certificate = Get-Item -LiteralPath "Cert:\CurrentUser\My\$thumbprint" -ErrorAction Stop
if (-not $certificate.HasPrivateKey) { throw 'The signing certificate has no accessible private key.' }
if ($certificate.NotBefore -gt (Get-Date) -or $certificate.NotAfter -le (Get-Date)) { throw 'The signing certificate is outside its validity period.' }
if ($certificate.Subject -eq $certificate.Issuer) { throw 'A self-signed certificate is not accepted by this release signing step.' }
if ($certificate.PublicKey.Oid.Value -ne '1.2.840.113549.1.1.1') { throw 'Use a trusted RSA code-signing certificate for this Smart App Control signing flow.' }
$ekuExtension = $certificate.Extensions | Where-Object { $_.Oid.Value -eq '2.5.29.37' } | Select-Object -First 1
if (-not $ekuExtension) { throw 'The certificate has no Enhanced Key Usage extension.' }
$eku = [System.Security.Cryptography.X509Certificates.X509EnhancedKeyUsageExtension]::new($ekuExtension, $ekuExtension.Critical)
$codeSigning = @($eku.EnhancedKeyUsages | Where-Object { $_.Value -eq '1.3.6.1.5.5.7.3.3' })
if ($codeSigning.Count -eq 0) { throw 'The certificate does not have the Code Signing enhanced key usage.' }
$chain = New-Object System.Security.Cryptography.X509Certificates.X509Chain
try {
    $chain.ChainPolicy.RevocationMode = [System.Security.Cryptography.X509Certificates.X509RevocationMode]::Online
    if (-not $chain.Build($certificate)) {
        $reason = ($chain.ChainStatus | ForEach-Object { $_.Status.ToString() }) -join ', '
        throw "The signing certificate chain could not be validated: $reason"
    }
} finally { $chain.Dispose() }
$artifacts = (Resolve-Path -LiteralPath $ArtifactsDirectory).Path
$files = @(
    (Join-Path $artifacts 'VST2\VocalBalance.dll'),
    (Join-Path $artifacts 'VST3\VocalBalance.vst3\Contents\x86_64-win\VocalBalance.vst3')
)
foreach ($file in $files) { if (-not (Test-Path -LiteralPath $file -PathType Leaf)) { throw "Missing artifact: $file" } }
if (-not $SignToolPath) {
    $command = Get-Command signtool.exe -ErrorAction SilentlyContinue
    if ($command) { $SignToolPath = $command.Source }
    else {
        $sdkRoot = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\bin'
        $candidate = Get-ChildItem -Path "$sdkRoot\*\x64\signtool.exe" -File |
            Sort-Object { [version]$_.Directory.Parent.Name } -Descending | Select-Object -First 1
        if ($candidate) { $SignToolPath = $candidate.FullName }
    }
}
if (-not $SignToolPath -or -not (Test-Path -LiteralPath $SignToolPath)) { throw 'Install Windows SDK SignTool or pass -SignToolPath.' }
# Select a private key from the certificate store. No password or key is passed on the command line.
& $SignToolPath sign /sha1 $thumbprint /fd SHA256 /tr $TimestampServer /td SHA256 @files
if ($LASTEXITCODE -ne 0) { throw "SignTool failed with exit code $LASTEXITCODE" }
$report = foreach ($file in $files) {
    $signature = Get-AuthenticodeSignature -LiteralPath $file
    if ($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Thumbprint -ne $thumbprint) { throw "Signature validation failed: $file" }
    if (-not $signature.TimeStamperCertificate) { throw "A trusted timestamp is missing: $file" }
    [pscustomobject]@{
        File = $file
        Status = $signature.Status.ToString()
        SignerThumbprint = $signature.SignerCertificate.Thumbprint
        SHA256 = (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash
    }
}
$report | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $artifacts 'SIGNATURES.json') -Encoding UTF8
$report | Format-Table -AutoSize
