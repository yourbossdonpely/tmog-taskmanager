$ErrorActionPreference = "Stop"

$root = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$version = $env:TMOG_VERSION
$qt = $env:QT_ROOT_DIR
if ([string]::IsNullOrWhiteSpace($version)) { throw "TMOG_VERSION is required" }
if ([string]::IsNullOrWhiteSpace($qt)) { throw "QT_ROOT_DIR is required" }

$dist = Join-Path $root "dist\windows"
$work = Join-Path $root "build\packaging\windows"
$installerStage = Join-Path $work "installer"
$portableStage = Join-Path $work "portable"
Remove-Item $dist, $work -Recurse -Force -ErrorAction SilentlyContinue
New-Item $dist, $installerStage, $portableStage -ItemType Directory -Force | Out-Null

$builtExe = Join-Path $root "build\Release\TaskManagerOG.exe"
if (!(Test-Path $builtExe)) { throw "Release executable not found at $builtExe" }
Copy-Item $builtExe (Join-Path $installerStage "TaskManagerOG.exe")
Copy-Item (Join-Path $root "LICENSE") $installerStage
Copy-Item (Join-Path $root "docs\NOTICE.md") (Join-Path $installerStage "NOTICE.md")

& (Join-Path $qt "bin\windeployqt.exe") `
    --release `
    --no-translations `
    --no-system-d3d-compiler `
    --no-opengl-sw `
    --compiler-runtime `
    (Join-Path $installerStage "TaskManagerOG.exe")
if ($LASTEXITCODE -ne 0) { throw "windeployqt failed" }

function Find-Tool([string]$name, [string]$base) {
    $found = Get-ChildItem -Path $base -Recurse -Filter $name -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -match "\\x64\\" } |
        Select-Object -First 1
    if ($null -eq $found) {
        $found = Get-ChildItem -Path $base -Recurse -Filter $name -ErrorAction SilentlyContinue |
            Select-Object -First 1
    }
    if ($null -eq $found) { throw "Could not find $name under $base" }
    return $found.FullName
}

$signtool = Find-Tool "signtool.exe" (Join-Path ${env:ProgramFiles(x86)} "Windows Kits\10\bin")
$cert = New-SelfSignedCertificate `
    -Type CodeSigningCert `
    -Subject "CN=Task Manager OG by Don PELY (self-signed)" `
    -CertStoreLocation "Cert:\CurrentUser\My" `
    -KeyAlgorithm RSA `
    -KeyLength 3072 `
    -HashAlgorithm SHA256 `
    -NotAfter (Get-Date).AddYears(3)
$pfx = Join-Path $work "selfsigned.pfx"
$pfxPassword = "tmog-local-signing"
$securePassword = ConvertTo-SecureString $pfxPassword -Force -AsPlainText
Export-PfxCertificate -Cert $cert -FilePath $pfx -Password $securePassword | Out-Null
Export-Certificate -Cert $cert -FilePath (Join-Path $dist "TaskManagerOG-$version-windows-selfsigned.cer") | Out-Null

function Sign-File([string]$file) {
    & $signtool sign /fd SHA256 /f $pfx /p $pfxPassword $file
    if ($LASTEXITCODE -ne 0) { throw "Signing failed for $file" }
    $signature = Get-AuthenticodeSignature $file
    Write-Host "Signed $file ($($signature.Status))"
}

Sign-File (Join-Path $installerStage "TaskManagerOG.exe")

Copy-Item (Join-Path $installerStage "*") $portableStage -Recurse
New-Item (Join-Path $portableStage "portable.flag") -ItemType File | Out-Null
Compress-Archive -Path (Join-Path $portableStage "*") `
    -DestinationPath (Join-Path $dist "TaskManagerOG-$version-windows-x64-portable.zip") `
    -CompressionLevel Optimal

$env:TMOG_STAGE = $installerStage
$env:TMOG_DIST = $dist
$isccCandidates = @(
    (Join-Path ${env:ProgramFiles(x86)} "Inno Setup 6\ISCC.exe"),
    (Join-Path $env:ProgramFiles "Inno Setup 6\ISCC.exe")
)
$iscc = $isccCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if ($null -eq $iscc) { throw "Inno Setup ISCC.exe was not found" }
& $iscc (Join-Path $root "packaging\windows\TaskManagerOG.iss")
if ($LASTEXITCODE -ne 0) { throw "Inno Setup failed" }

$wixBase = Join-Path ${env:ProgramFiles(x86)} "WiX Toolset v3.14\bin"
$heat = Find-Tool "heat.exe" $wixBase
$candle = Find-Tool "candle.exe" $wixBase
$light = Find-Tool "light.exe" $wixBase
& $heat dir $installerStage -cg AppFiles -gg -scom -sreg -sfrag -srd `
    -dr INSTALLDIR -var var.SourceDir -out (Join-Path $work "AppFiles.wxs")
if ($LASTEXITCODE -ne 0) { throw "WiX heat failed" }
# heat emits 32-bit components even though this package targets Program Files.
# Mark every harvested component as 64-bit so ICE80 does not misclassify it.
$appFiles = Get-Content (Join-Path $work "AppFiles.wxs") -Raw
$appFiles = $appFiles -replace '<Component ', '<Component Win64="yes" '
Set-Content (Join-Path $work "AppFiles.wxs") $appFiles -NoNewline
& $candle -dSourceDir="$installerStage" -dProductVersion="$version" `
    -out "$work\" (Join-Path $root "packaging\windows\TaskManagerOG.wxs")
if ($LASTEXITCODE -ne 0) { throw "WiX candle failed for the product" }
& $candle -dSourceDir="$installerStage" -dProductVersion="$version" `
    -out "$work\" (Join-Path $work "AppFiles.wxs")
if ($LASTEXITCODE -ne 0) { throw "WiX candle failed for the files" }
$msi = Join-Path $dist "TaskManagerOG-$version-windows-x64.msi"
& $light -arch x64 -out $msi (Join-Path $work "TaskManagerOG.wixobj") (Join-Path $work "AppFiles.wixobj")
if ($LASTEXITCODE -ne 0) { throw "WiX light failed" }

Sign-File (Join-Path $dist "TaskManagerOG-$version-windows-x64-setup.exe")
Sign-File $msi

Get-ChildItem $dist | Select-Object Name, Length
