param(
    [string]$SourceDir = ".\\sound_server\\audio",
    [string]$OutputDir = ".\\sound_server\\audio_normalized",
    [int]$SampleRate = 22050
)

$ffmpeg = Get-Command ffmpeg -ErrorAction SilentlyContinue
if (-not $ffmpeg) {
    $fallback = "C:\Users\alanb\AppData\Local\ffmpeg\ffmpeg-8.1.2-full_build\bin\ffmpeg.exe"
    if (Test-Path -LiteralPath $fallback) {
        $ffmpeg = @{ Source = $fallback }
    } else {
        Write-Error "ffmpeg was not found on PATH and the fallback path does not exist: $fallback"
        exit 1
    }
}

if (-not (Test-Path -LiteralPath $SourceDir)) {
    Write-Error "SourceDir not found: $SourceDir"
    exit 1
}

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

$wavFiles = Get-ChildItem -LiteralPath $SourceDir -Filter *.wav
if (-not $wavFiles) {
    Write-Warning "No WAV files found in $SourceDir"
    exit 0
}

foreach ($file in $wavFiles) {
    $target = Join-Path $OutputDir $file.Name
    Write-Host "Normalizing $($file.Name) -> $target"
    & $ffmpeg.Source `
        -y `
        -i $file.FullName `
        -map_metadata -1 `
        -ac 2 `
        -ar $SampleRate `
        -c:a pcm_s16le `
        $target

    if ($LASTEXITCODE -ne 0) {
        Write-Error "ffmpeg failed for $($file.FullName)"
        exit $LASTEXITCODE
    }
}

Write-Host "Done. Normalized WAV files written to $OutputDir"
