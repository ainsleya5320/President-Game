param([switch]$Editor,[switch]$Review,[switch]$Preview,[switch]$AuditKeys)
$projectRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$engine = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$project = Join-Path $projectRoot 'OvalOffice.uproject'
$shaderTemp = Join-Path $projectRoot 'Saved\AF1ShaderTemp'
$shaderWorking = Join-Path $projectRoot 'Saved\AF1ShaderWorking'
$zenData = Join-Path $projectRoot 'Saved\AF1ZenData'
foreach ($directory in @($shaderTemp,$shaderWorking,$zenData)) { New-Item -ItemType Directory -Path $directory -Force | Out-Null }
$env:TEMP = $shaderTemp
$env:TMP = $shaderTemp
$arguments = @(('"'+$project+'"'),'/Game/AirForceOne/Maps/AirForceOne','-dx11','-windowed','-ResX=1280','-ResY=720',('-ZenDataPath="'+$zenData+'"'),('-ShaderWorkingDir="'+$shaderWorking+'"'),'-DDC-ForceMemoryCache','-NoSendLog')
if (!$Editor) { $arguments += '-game' }
if ($Review) { $arguments += ('"-ExecCmds=py '+(Join-Path $PSScriptRoot 'test_air_force_one.py').Replace('\','/')+'"') }
if ($Preview) { $arguments += ('"-ExecCmds=py '+(Join-Path $PSScriptRoot 'preview_air_force_one.py').Replace('\','/')+'"') }
if ($AuditKeys) { $arguments += ('"-ExecCmds=py '+(Join-Path $PSScriptRoot 'audit_air_force_one_keys.py').Replace('\','/')+'"') }
# The game/editor is the interactive result requested by the user.
Start-Process -FilePath $engine -ArgumentList $arguments -WindowStyle Normal -PassThru | Select-Object Id
