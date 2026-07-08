param(
    [string]$SourceDir = ".\sound_server\audio_originals",
    [string]$OutputDir = "",
    [int]$SampleRate = 22050
)

if ([string]::IsNullOrWhiteSpace($OutputDir)) {
    $OutputDir = if ($SampleRate -eq 22050) {
        ".\sound_server\audio"
    } else {
        ".\sound_server\audio_$SampleRate"
    }
}

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

$sourceFiles = Get-ChildItem -LiteralPath $SourceDir -Filter *.ogg | Sort-Object Name
if (-not $sourceFiles) {
    Write-Warning "No OGG files found in $SourceDir"
    exit 0
}

foreach ($file in $sourceFiles) {
    $targetName = [System.IO.Path]::GetFileNameWithoutExtension($file.Name) + ".wav"
    $target = Join-Path $OutputDir $targetName
    Write-Host "Converting $($file.Name) -> $targetName"
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

Write-Host "Done. Converted WAV files written to $OutputDir"
