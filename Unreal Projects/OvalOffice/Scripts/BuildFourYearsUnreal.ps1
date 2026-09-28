param([string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.8')
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$project = Join-Path $projectRoot 'OvalOffice.uproject'
# Some launcher installations on ARM Windows include only the x64 .NET runtime.
$runtimeRoot = Join-Path $EngineRoot 'Engine\Binaries\ThirdParty\DotNet\10.0'
$architectures = if ($env:PROCESSOR_ARCHITECTURE -eq 'ARM64') { @('win-arm64','win-x64') } else { @('win-x64') }
$dotnet = $architectures | ForEach-Object { Join-Path $runtimeRoot "$_\dotnet.exe" } | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
if (!$dotnet) { throw "No bundled .NET runtime found in $runtimeRoot" }
$buildTool = Join-Path $EngineRoot 'Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll'
Push-Location (Join-Path $EngineRoot 'Engine\Source')
try {
    & $dotnet $buildTool OvalOfficeEditor Win64 Development ("-Project=$project") -WaitMutex -NoHotReloadFromIDE -MaxParallelActions=2
    if ($LASTEXITCODE -ne 0) { throw "Unreal build failed with exit code $LASTEXITCODE. See the UnrealBuildTool log." }
} finally { Pop-Location }
