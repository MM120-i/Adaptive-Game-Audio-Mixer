param(
    [string]$Config = "Debug"
)

$ErrorActionPreference = "Stop"
$buildDir = "build"

Write-Host "==> Configuring CMake with MSVC /analyze..." -ForegroundColor Cyan
cmake -S . -B $buildDir -G "Visual Studio 18 2026" -A x64 -DENABLE_ANALYZE=ON
if ($LASTEXITCODE -ne 0) { Write-Host "CONFIGURE FAILED" -ForegroundColor Red; exit $LASTEXITCODE }

Write-Host "==> Building with code analysis ($Config)..." -ForegroundColor Cyan
cmake --build $buildDir --config $Config --parallel 2>&1 | Tee-Object -Variable buildOutput

$warnings = ($buildOutput | Select-String "warning C6").Count
$errors   = ($buildOutput | Select-String "error C").Count

Write-Host ""
Write-Host ("MSVC /analyze:  {0} warnings  |  {1} errors" -f $warnings, $errors) -ForegroundColor $(if($errors -eq 0){"Green"}else{"Red"})

# Optional: cppcheck deeper analysis
if (Get-Command cppcheck -ErrorAction SilentlyContinue) {
    Write-Host ""
    Write-Host "==> Running cppcheck..." -ForegroundColor Cyan
    $cppcheckOutput = & cppcheck --enable=all --check-level=exhaustive --suppress=missingInclude --suppress=missingIncludeSystem --suppress=unusedFunction --suppress=checkersReport -I src/ src/ 2>&1
    $cppcheckOutput | ForEach-Object { Write-Host $_ }
    $issues = ($cppcheckOutput -match "^(warning|error|style|performance|portability):" | Measure-Object).Count
    Write-Host ""
    if ($issues -eq 0) {
        Write-Host "cppcheck:       0 issues" -ForegroundColor Green
    } else {
        Write-Host ("cppcheck:       {0} issues found" -f $issues) -ForegroundColor Yellow
    }
} else {
    Write-Host ""
    Write-Host "cppcheck not installed. Install via: winget install cppcheck" -ForegroundColor DarkGray
}

Write-Host "==> Restoring build config (no /analyze for speed)..." -ForegroundColor Cyan
cmake -S . -B $buildDir -DENABLE_ANALYZE=OFF > $null 2>&1

if ($errors -gt 0) { exit 1 } else { exit 0 }
