param(
    [int]$Port = 8000
)

$repoRoot = Split-Path -Parent $PSScriptRoot
$url = "http://localhost:$Port/tools/webserial_trigger.html"

$pythonCommand = Get-Command py -ErrorAction SilentlyContinue
if (-not $pythonCommand) {
    $pythonCommand = Get-Command python -ErrorAction SilentlyContinue
}

if (-not $pythonCommand) {
    Write-Error "Neither 'py' nor 'python' was found on PATH."
    exit 1
}

Write-Host ""
Write-Host "Starting local web server from: $repoRoot"
Write-Host "Open this URL in a Chromium-based browser:"
Write-Host $url
Write-Host ""
Write-Host "Press Ctrl+C to stop the server."
Write-Host ""

Set-Location -LiteralPath $repoRoot

if ($pythonCommand.Name -eq "py.exe" -or $pythonCommand.Name -eq "py") {
    & $pythonCommand.Source -m http.server $Port
} else {
    & $pythonCommand.Source -m http.server $Port
}
