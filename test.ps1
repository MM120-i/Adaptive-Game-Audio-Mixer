param(
    [string]$Config = "Debug"
)

$ErrorActionPreference = "Stop"
$buildDir = "build"

Write-Host "==> Building tests ($Config)..." -ForegroundColor Cyan
cmake --build $buildDir --config $Config --target AudioMixerTests --parallel
if ($LASTEXITCODE -ne 0) {
    Write-Host "BUILD FAILED" -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Host "==> Running tests..." -ForegroundColor Cyan
Write-Host ""

$testExe = "$buildDir\$Config\AudioMixerTests.exe"
$result = & $testExe 2>&1

$passed = 0
$failed = 0

foreach ($line in $result) {
    if ($line -match "^Completed tests in ") {
        $passed++
        Write-Host "  PASS  " -NoNewline -ForegroundColor Green
        Write-Host ($line -replace "^Completed tests in ", "")
    }
    elseif ($line -match "FAIL") {
        $failed++
        Write-Host "  FAIL  " -NoNewline -ForegroundColor Red
        Write-Host ($line)
    }
    elseif ($line -match "^(Starting|Random|-----)") {
    }
    elseif ($line.Trim()) {
        Write-Host $line
    }
}

Write-Host ""
$color = if ($failed -eq 0) { "Green" } else { "Red" }
Write-Host ("Total:  {0} passed  |  {1} failed" -f $passed, $failed) -ForegroundColor $color

if ($failed -gt 0) { 
    exit 1 
} 
else { 
    exit 0
}
