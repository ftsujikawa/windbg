<#
.SYNOPSIS
    Q01 (docs/04_テスト仕様書.md 6章): rebuilds tdb via make.bat and fails
    if the compiler emitted any warnings. Run separately from
    run_tests.ps1 since it triggers a full rebuild.

.NOTES
    Requires a Developer Command Prompt environment (cl/win_bison/win_flex
    on PATH). Run from a "x64 Native Tools Command Prompt", or call
    vcvarsall.bat first, e.g.:

        cmd /c '"<VS install>\VC\Auxiliary\Build\vcvarsall.bat" x64 && ' + `
               'powershell -File tests\check_build_warnings.ps1'
#>

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$binDir   = $repoRoot

Push-Location $binDir
try {
    $output = cmd /c "make.bat 2>&1"
} finally {
    Pop-Location
}

$outputText = ($output -join "`n")
$warnings = [regex]::Matches($outputText, '(?m)^.*: warning [A-Z]+\d+:.*$')

if ($warnings.Count -eq 0) {
    Write-Host "[PASS] Q01   コンパイル警告なし" -ForegroundColor Green
    exit 0
} else {
    Write-Host "[FAIL] Q01   コンパイル警告なし" -ForegroundColor Red
    foreach ($w in $warnings) { Write-Host ("        " + $w.Value) -ForegroundColor Yellow }
    exit 1
}
