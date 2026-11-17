param(
    [string]$Compiler = "$PSScriptRoot/work/toolchain/w64devkit/bin/gcc.exe",
    [string]$SDL = "$PSScriptRoot/work/toolchain/SDL3-3.4.20/x86_64-w64-mingw32",
    [switch]$Test,
    [string]$Output = 'build/sr2-enhanced.exe'
)
$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    New-Item -ItemType Directory -Force build | Out-Null
    $common = @('-std=c11', '-O2', '-g', '-Wall', '-Wextra', '-Wno-unused-parameter',
        '-Wno-format-truncation', '-fno-strict-aliasing', '-Isr2port/src',
        '-Isr2port/third_party/nuked-opl3', "-I$SDL/include")
    $sources = @(Get-ChildItem sr2port/src -Recurse -Filter '*.c' | ForEach-Object FullName)
    & $Compiler @common @sources sr2port/third_party/nuked-opl3/opl3.c "-L$SDL/lib" -lSDL3 -lm -o $Output
    if ($LASTEXITCODE -ne 0) { throw 'Game compilation failed' }
    if (!(Test-Path build/SDL3.dll) -or (Get-FileHash "$SDL/bin/SDL3.dll").Hash -ne (Get-FileHash build/SDL3.dll).Hash) {
        Copy-Item "$SDL/bin/SDL3.dll" build/SDL3.dll
    }
    if ($Test) {
        & $Compiler @common sr2port/tests/enhanced_test.c sr2port/src/enhanced/enhanced.c sr2port/src/enhanced/controls.c sr2port/src/enhanced/sprites.c -lm -o build/enhanced-test.exe
        if ($LASTEXITCODE -ne 0) { throw 'Test compilation failed' }
        & ./build/enhanced-test.exe
        if ($LASTEXITCODE -ne 0) { throw 'Tests failed' }
        $testSources = @($sources | Where-Object { [IO.Path]::GetFileName($_) -notin @('main.c', 'race_road.c') })
        & $Compiler @common sr2port/tests/difficulty_test.c sr2port/tests/scenery_test.c @testSources sr2port/third_party/nuked-opl3/opl3.c "-L$SDL/lib" -lSDL3 -lm -o build/difficulty-test.exe
        if ($LASTEXITCODE -ne 0) { throw 'Difficulty test compilation failed' }
        & ./build/difficulty-test.exe
        if ($LASTEXITCODE -ne 0) { throw 'Difficulty tests failed' }
        & $Compiler @common sr2port/tests/audio_test.c sr2port/src/host.c sr2port/src/sound/adlib.c sr2port/src/sound/mus.c sr2port/third_party/nuked-opl3/opl3.c "-L$SDL/lib" -lSDL3 -lm -o build/audio-test.exe
        if ($LASTEXITCODE -ne 0) { throw 'Audio test compilation failed' }
        Push-Location build
        try {
            & ./audio-test.exe
            if ($LASTEXITCODE -ne 0) { throw 'Audio tests failed' }
        } finally { Pop-Location }
    }
} finally { Pop-Location }
