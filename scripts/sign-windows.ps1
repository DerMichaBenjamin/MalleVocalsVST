param([Parameter(Mandatory=$true)][string]$ArtifactDirectory)
$ErrorActionPreference='Stop'
if (-not $env:SIGN_CERT_THUMBPRINT) { throw 'Set SIGN_CERT_THUMBPRINT for an existing local code-signing certificate. Do not store private keys in the repository.' }
Get-ChildItem -LiteralPath $ArtifactDirectory -Recurse -File | Where-Object { $_.Extension -in '.vst3','.exe','.dll' } | ForEach-Object {
  & signtool sign /sha1 $env:SIGN_CERT_THUMBPRINT /fd SHA256 /tr 'http://timestamp.digicert.com' /td SHA256 $_.FullName
  if ($LASTEXITCODE -ne 0) { throw "Signing failed: $($_.FullName)" }
  & signtool verify /pa $_.FullName
  if ($LASTEXITCODE -ne 0) { throw "Signature verification failed: $($_.FullName)" }
}
