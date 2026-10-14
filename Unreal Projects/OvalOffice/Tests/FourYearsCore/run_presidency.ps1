param([string]$Compiler = 'cl.exe')
$ErrorActionPreference = 'Stop'
$project = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../..')).Path
$output = Join-Path $project 'Saved/PresidencyTests'
New-Item -ItemType Directory -Path $output -Force | Out-Null
Push-Location $output
try {
    $public = Join-Path $project 'Source/OvalOffice/Public'
    $core = Join-Path $project 'Source/OvalOffice/Private/FourYears/Core'
    $sources = @((Join-Path $PSScriptRoot 'presidency_main.cpp'))
    foreach ($file in @('FourYearsJson.cpp','FourYearsSim.cpp','FourYearsElectoral.cpp','FourYearsDiplomacy.cpp','FourYearsPresidency.cpp')) { $sources += Join-Path $core $file }
    & $Compiler /nologo /std:c++17 /O2 /EHsc /W4 /I $public @sources /Fe:presidency_tests.exe
    if ($LASTEXITCODE -ne 0) { throw 'Campaign test compilation failed.' }
    & (Join-Path $output 'presidency_tests.exe') (Join-Path $project 'Prototype/data')
    if ($LASTEXITCODE -ne 0) { throw 'Campaign regression test failed.' }
} finally { Pop-Location }
