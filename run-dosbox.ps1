$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$appName = if ($env:OUTPUT_EXE) { $env:OUTPUT_EXE } else { "program.exe" }
$appPath = Join-Path $scriptDir "bin\$appName"

function Find-Dosbox {
    $candidates = @(
        "dosbox",
        "dosbox-x"
    )

    foreach ($candidate in $candidates) {
        $cmd = Get-Command $candidate -ErrorAction SilentlyContinue
        if ($cmd) {
            return $cmd.Source
        }
    }

    $commonPaths = @(
        "C:\Program Files\DOSBox\DOSBox.exe",
        "C:\Program Files (x86)\DOSBox\DOSBox.exe",
        "C:\Program Files\DOSBox-X\DOSBox.exe",
        "C:\Program Files (x86)\DOSBox-X\DOSBox.exe"
    )

    foreach ($path in $commonPaths) {
        if (Test-Path $path) {
            return $path
        }
    }

    return $null
}

if (-not (Test-Path $appPath)) {
    if (Get-Command docker -ErrorAction SilentlyContinue) {
        Write-Host "Binary not found; building with Docker..."
        & (Join-Path $scriptDir "build-docker.sh")
    } elseif (Get-Command wcl -ErrorAction SilentlyContinue) {
        Write-Host "Binary not found; building with local Open Watcom..."
        & (Join-Path $scriptDir "build.sh")
    } else {
        throw "Neither Docker nor Open Watcom is available. Install a toolchain or build the project first."
    }
}

$dosboxPath = Find-Dosbox
if (-not $dosboxPath) {
    throw "DOSBox was not found in PATH or common install locations."
}

$configPath = Join-Path $env:TEMP "dosbox-template-$PID.conf"
@"
[sdl]
autolock=true

[dosbox]
memsize=64

[cpu]
cycles=max

[autoexec]
mount c "$scriptDir"
c:
cd \bin
$appName
echo Press Ctrl-F9 to exit DOSBox
"@ | Set-Content -Path $configPath -Encoding ASCII

Write-Host "Launching DOSBox with $appPath"
& $dosboxPath -conf $configPath
Remove-Item $configPath -ErrorAction SilentlyContinue
