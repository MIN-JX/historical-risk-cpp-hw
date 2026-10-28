param([string]$BuildDirectory = "build")
$ErrorActionPreference = "Stop"
$ProjectDirectory = Split-Path -Parent $PSScriptRoot
Push-Location $ProjectDirectory
try {
    & cmake -S . -B $BuildDirectory -DCMAKE_BUILD_TYPE=Release
    if ($LASTEXITCODE -ne 0) { throw "CMake configuration failed: $LASTEXITCODE" }
    & cmake --build $BuildDirectory --config Release --parallel 2
    if ($LASTEXITCODE -ne 0) { throw "Build failed: $LASTEXITCODE" }
    & ctest --test-dir $BuildDirectory -C Release --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw "CTest failed: $LASTEXITCODE" }
    & cmake --build $BuildDirectory --config Release --target run_benchmarks
    if ($LASTEXITCODE -ne 0) { throw "Benchmark validation failed: $LASTEXITCODE" }
    & cmake --build $BuildDirectory --config Release --target docs
    if ($LASTEXITCODE -ne 0) { throw "Documentation failed: $LASTEXITCODE" }
    Write-Host "Verification complete: build, tests, benchmark JSON and documentation passed."
}
finally { Pop-Location }
