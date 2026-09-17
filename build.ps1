$sourceFolder = if ($env:SRC_DIR) { $env:SRC_DIR } else { ".\src" }
$outputFolder = if ($env:BIN_DIR) { $env:BIN_DIR } else { ".\bin" }
$objFolder = if ($env:OBJ_DIR) { $env:OBJ_DIR } else { ".\obj" }
$errorFolder = if ($env:ERR_DIR) { $env:ERR_DIR } else { ".\err" }
$outputFileName = if ($env:OUTPUT_EXE) { $env:OUTPUT_EXE } else { "program.exe" }

if (-not (Test-Path $sourceFolder)) {
    Write-Error "Source directory '$sourceFolder' does not exist."
    exit 1
}

foreach ($dir in @($outputFolder, $objFolder, $errorFolder)) {
    if (-not (Test-Path $dir)) {
        New-Item -ItemType Directory -Path $dir -Force | Out-Null
    }
}

Get-ChildItem -Path $outputFolder, $objFolder, $errorFolder -Force -ErrorAction SilentlyContinue | Remove-Item -Recurse -Force

if (-not (Get-Command wcl -ErrorAction SilentlyContinue)) {
    Write-Error "Open Watcom 'wcl' was not found in PATH. Install Open Watcom or run the Docker build helper."
    exit 1
}

$sourceFiles = Get-ChildItem -Path $sourceFolder -Include *.c,*.cpp -File -ErrorAction SilentlyContinue
if (-not $sourceFiles) {
    Write-Error "No C/C++ source files found in $sourceFolder"
    exit 1
}

Write-Host "Compiling the following source files for DOS:"
$sourceFiles | ForEach-Object { Write-Host $_.FullName }

$compileArgs = @("-bt=dos", "-zq", "-k32768")
if ($env:REAL_HARDWARE -eq "1") {
    $compileArgs += "-dREAL_HARDWARE=1"
}
if ($env:INCLUDE) {
    $compileArgs += "-I$env:INCLUDE"
}
$compileArgs += @("-fe:$outputFolder\$outputFileName", "-fo:$objFolder\")
$compileArgs += $sourceFiles.FullName

& wcl @compileArgs 2> "$errorFolder\build_errors.log"
$compileStatus = $LASTEXITCODE

Get-ChildItem -Path "." -Filter *.err -File -ErrorAction SilentlyContinue | ForEach-Object {
    Move-Item -Path $_.FullName -Destination $errorFolder -Force
}

if ($compileStatus -eq 0 -and (Test-Path (Join-Path $outputFolder $outputFileName))) {
    Write-Host "Compilation successful. Output: $(Join-Path $outputFolder $outputFileName)"
    if ((Get-Item "$errorFolder\build_errors.log" -ErrorAction SilentlyContinue).Length -eq 0) {
        Remove-Item "$errorFolder\build_errors.log" -Force -ErrorAction SilentlyContinue
    }
    exit 0
}

Write-Error "Compilation failed. Check $errorFolder\build_errors.log"
exit 1
