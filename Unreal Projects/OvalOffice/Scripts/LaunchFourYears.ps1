param([switch]$NoBrowser)
$projectRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$webRoot = Join-Path $projectRoot 'Prototype'
$url = 'http://127.0.0.1:8781/Four_Years_Prototype.html'
$running = $false
try {
    $response = Invoke-WebRequest -Uri $url -TimeoutSec 3
    if ($response.Content -notmatch '<title>Four Years') { throw 'Port 8781 is serving another application.' }
    $running = $true
} catch {
    if ($_.Exception.Message -eq 'Port 8781 is serving another application.') { throw }
}
if (!$running) {
    $python = Join-Path $env:USERPROFILE '.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
    if (!(Test-Path -LiteralPath $python)) {
        $command = Get-Command python.exe -ErrorAction SilentlyContinue
        if (!$command -or $command.Source -match 'WindowsApps') { throw 'Install Python 3, then run this launcher again.' }
        $python = $command.Source
    }
    $logs = Join-Path $projectRoot 'Saved\BrowserPrototype'
    New-Item -ItemType Directory -Path $logs -Force | Out-Null
    $arguments = @('-m','http.server','8781','--bind','127.0.0.1','--directory',('"'+$webRoot+'"'))
    Start-Process -FilePath $python -ArgumentList $arguments -WindowStyle Hidden -RedirectStandardOutput (Join-Path $logs 'server.log') -RedirectStandardError (Join-Path $logs 'requests.log') | Out-Null
    $ready = $false
    for ($attempt = 0; $attempt -lt 20; $attempt++) {
        try { $response = Invoke-WebRequest -Uri $url -TimeoutSec 2; $ready = $response.Content -match '<title>Four Years'; if ($ready) { break } } catch { }
        Start-Sleep -Milliseconds 200
    }
    if (!$ready) { throw 'The local game server did not start. Check Saved/BrowserPrototype/requests.log.' }
}
if (!$NoBrowser) { Start-Process $url }
Write-Output $url
