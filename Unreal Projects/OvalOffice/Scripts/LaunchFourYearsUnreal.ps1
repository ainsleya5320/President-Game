param([switch]$Editor)
# Opens the native Four Years slice: walk the Oval Office and govern from the Resolute Desk.
# The C++ module must be built first (see Source/README.md).
$projectRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$engine = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$project = Join-Path $projectRoot 'OvalOffice.uproject'
if (!(Test-Path -LiteralPath $engine)) { throw "Unreal Engine 5.8 was not found at $engine. Adjust the path in this script." }
$module = Join-Path $projectRoot 'Binaries\Win64\UnrealEditor-OvalOffice.dll'
if (!(Test-Path -LiteralPath $module)) { Write-Warning 'The OvalOffice C++ module has not been built yet. Unreal will offer to build it; Visual Studio 2022 with C++ game development is required.' }
$arguments = @(('"'+$project+'"'),'/Game/OvalOffice/Maps/OvalOffice','-dx11','-windowed','-ResX=1280','-ResY=720','-NoSendLog')
if (!$Editor) { $arguments += '-game' }
Start-Process -FilePath $engine -ArgumentList $arguments -WindowStyle Normal -PassThru | Select-Object Id
