param([switch]$VerifyPackage,[switch]$ListPorts)
$ErrorActionPreference='Stop'
$propRoot=$PSScriptRoot
$propTool=Join-Path $propRoot 'installer-tools\esptool.exe'
function Resolve-HauntCueFamily([string]$info) {
 if($info -match 'ESP32-C3'){'esp32c3'}elseif($info -match 'ESP32-S3'){'esp32s3'}elseif($info -match 'ESP32-S2'){'esp32s2'}elseif($info -match 'ESP32-C6'){'esp32c6'}elseif($info -match 'ESP32-(C|H|P)'){throw 'This chip family has no compatible HauntCue build.'}elseif($info -match 'ESP32'){'esp32'}else{throw 'Unsupported chip; no firmware written.'}
}
try {
 if(!(Test-Path -LiteralPath $propTool)){throw 'Extract the entire ZIP first. Installer tool is missing.'}
 $profiles=@(Get-Content -LiteralPath (Join-Path $propRoot 'BOARD-PROFILES.json') -Raw | ConvertFrom-Json | ForEach-Object { $_ })
 if($profiles.Count -ne 5){throw 'Missing board-family profiles. Extract the entire package again.'}
 foreach($profile in $profiles){
  $dir=Join-Path $propRoot $profile.directory
  $manifest=Get-Content -LiteralPath (Join-Path $dir 'SHA256.json') -Raw | ConvertFrom-Json
  foreach($name in @('bootloader.bin','partitions.bin','boot_app0.bin','HauntCue.bin')){
   $p=Join-Path $dir $name
   $wanted=($manifest | Where-Object {$_.File -eq $name}).Hash
   if(!(Test-Path -LiteralPath $p) -or !$wanted -or (Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash -ne $wanted){throw "Missing or invalid firmware: $($profile.chip)/$name"}
  }
  if((Get-Item -LiteralPath (Join-Path $dir 'HauntCue.bin')).Length -gt $profile.maxAppBytes){throw "Application exceeds partition: $($profile.chip)"}
 }
 if($VerifyPackage){Write-Host 'PASS: all five family builds, size limits, firmware hashes and installer tool';exit 0}
 $ports=@([System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object)
 if($ListPorts){$ports;exit 0}
 Write-Host "`nHauntCue - multi-board Windows installer" -ForegroundColor Green
 Write-Host 'Rick Scarlett - Free for fellow fiends | rick.scarlett@gmail.com'
 Write-Host 'Optional donations: $rickscarlett | https://cash.app/$rickscarlett'
 Write-Host 'Connect by USB only. Disconnect loads and close serial monitors.'
 if(!$ports.Count){throw 'No COM ports found. Try a USB data cable and the official board USB driver.'}
 Write-Host "Available ports: $($ports -join ', ')"
 $suggested=if($ports.Count -eq 1){$ports[0]}else{''}
 $port=Read-Host "Board COM port [$suggested]";if(!$port){$port=$suggested}
 if($ports -notcontains $port){throw 'Choose a listed COM port.'}
 $identity=@(& $propTool --chip auto --port $port --baud 115200 flash-id 2>&1)
 if($LASTEXITCODE -ne 0){throw 'Cannot detect board. Hold BOOT, tap RESET, release BOOT and retry.'}
 $info=$identity -join "`n";$identity | ForEach-Object {Write-Host $_}
 $chip=Resolve-HauntCueFamily $info
 if($info -notmatch 'Detected flash size:\s*(4|8|16|32)\s*MB'){throw 'At least 4MB flash is required; no firmware written.'}
 $profile=$profiles | Where-Object {$_.chip -eq $chip};if(!$profile){throw 'Missing family profile; no firmware written.'}
 Write-Host "Detected $($profile.name): $($profile.status)" -ForegroundColor Yellow
 for($i=0;$i -lt $profile.boards.Count;$i++){Write-Host "$($i+1) = $($profile.boards[$i])"}
 $board=Read-Host 'Carrier layout number [1]';if(!$board){$board='1'}
 if($board -notmatch '^\d+$' -or [int]$board -lt 1 -or [int]$board -gt $profile.boards.Count){throw 'Choose a listed carrier layout.'}
 Write-Host "Selected $($profile.boards[[int]$board-1]). Confirm printed GPIO labels in the web wiring guide."
 Write-Host "Recommended: $($profile.outputs) switched outputs, 2 lighting, 1 servo, $($profile.audioPlayers) serial audio player(s)."
 Write-Host 'After installation, the setup wizard and Pins & trigger let you customize channels.'
 Write-Host '1 = Update and keep saved settings | 2 = Fresh installation (erase settings)'
 $mode=Read-Host 'Choose [1]';if(!$mode){$mode='1'};if($mode -notin @('1','2')){throw 'Choose 1 or 2.'}
 if($mode -eq '2'){$answer=Read-Host 'Type ERASE to delete all saved board data';if($answer -cne 'ERASE'){throw 'Cancelled.'}}
 elseif($chip -ne 'esp32'){Write-Host 'Keep settings is for an existing HauntCue build with the same family/partition layout. For first install or different firmware, choose fresh installation.' -ForegroundColor Yellow}
 $ready=Read-Host 'Type INSTALL to write firmware';if($ready -cne 'INSTALL'){throw 'Cancelled.'}
 if($mode -eq '2'){& $propTool --chip $chip --port $port --baud 115200 erase-flash;if($LASTEXITCODE -ne 0){throw 'Erase failed.'}}
 $dir=Join-Path $propRoot $profile.directory
 & $propTool --chip $chip --port $port --baud 115200 write-flash --flash-mode dio --flash-freq $profile.flashFrequency --flash-size 4MB $profile.bootOffset (Join-Path $dir 'bootloader.bin') 0x8000 (Join-Path $dir 'partitions.bin') 0xe000 (Join-Path $dir 'boot_app0.bin') 0x10000 (Join-Path $dir 'HauntCue.bin')
 if($LASTEXITCODE -ne 0){throw 'Upload failed; do not connect prop loads.'}
 Write-Host 'UPLOAD VERIFIED. Controller starts DISARMED.' -ForegroundColor Green
 Write-Host 'Join HauntCue-XXXXXX Wi-Fi; fresh password hauntcue123.'
 Write-Host 'Open http://192.168.4.1 ; fresh login admin / hauntcue123.'
 Write-Host 'Follow the wizard; choose your exact board in Pins & trigger > Where the wires go.'
 Write-Host 'Updates preserve existing credentials. Experimental families require USB-only testing before connecting loads.'
} catch {Write-Host $_.Exception.Message -ForegroundColor Red;exit 1}


