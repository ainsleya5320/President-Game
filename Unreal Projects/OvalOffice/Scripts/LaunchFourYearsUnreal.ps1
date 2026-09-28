param([switch]$Editor, [switch]$CabinetRoom)
# Opens the native Four Years slice: walk the Oval Office and govern from the Resolute Desk.
# The C++ module must be built first (see Source/README.md).
$projectRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$engine = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$project = Join-Path $projectRoot 'OvalOffice.uproject'
if (!(Test-Path -LiteralPath $engine)) { throw "Unreal Engine 5.8 was not found at $engine. Adjust the path in this script." }
$module = Join-Path $projectRoot 'Binaries\Win64\UnrealEditor-OvalOffice.dll'
if (!(Test-Path -LiteralPath $module)) { Write-Warning 'The OvalOffice C++ module has not been built yet. Unreal will offer to build it; Visual Studio 2022 with C++ game development is required.' }
$shaderTemp = Join-Path $projectRoot 'Saved\FourYearsShaderTemp'
$shaderWorking = Join-Path $projectRoot 'Saved\FourYearsShaderWorking'
$zenData = Join-Path $projectRoot 'Saved\FourYearsZenData'
foreach ($directory in @($shaderTemp,$shaderWorking,$zenData)) { New-Item -ItemType Directory -Path $directory -Force | Out-Null }
$env:TEMP = $shaderTemp
$env:TMP = $shaderTemp
# Keep editor subprocesses (Turnkey, project generation, later C++ builds) on an
# installed runtime too; the stock launcher assumes a bundled ARM runtime exists.
$dotnetRoot = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\ThirdParty\DotNet\10.0\win-x64'
if (Test-Path -LiteralPath (Join-Path $dotnetRoot 'dotnet.exe')) {
    $env:PATH = $dotnetRoot + ';' + $env:PATH
    $env:DOTNET_ROOT = $dotnetRoot
    $env:UE_USE_SYSTEM_DOTNET = '1'
    $env:DOTNET_ROLL_FORWARD = 'LatestMajor'
}
$level = if ($CabinetRoom) { '/Game/CabinetRoom/Maps/CabinetRoom' } else { '/Game/OvalOffice/Maps/OvalOffice' }
$map = if ($Editor) { $level } else { $level + '?game=/Script/OvalOffice.FourYearsGameMode' }
$arguments = @(('"'+$project+'"'),$map,'-dx11','-windowed','-ResX=1280','-ResY=720',('-ZenDataPath="'+$zenData+'"'),('-ShaderWorkingDir="'+$shaderWorking+'"'),'-DDC-ForceMemoryCache','-NoSendLog')
if ($Editor -and !$CabinetRoom) { $arguments += ('"-ExecCmds=py '+(Join-Path $PSScriptRoot 'configure_four_years.py').Replace('\','/')+'"') }
if (!$Editor) { $arguments += '-game' }
Start-Process -FilePath $engine -ArgumentList $arguments -WindowStyle Normal -PassThru | Select-Object Id
