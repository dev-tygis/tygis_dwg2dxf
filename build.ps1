# tygis_dwg2dxf 빌드
#
#   powershell -ExecutionPolicy Bypass -File build.ps1
#   powershell -ExecutionPolicy Bypass -File build.ps1 -Clean
#
# 필요한 것: Visual Studio 2022 이상 + "C++를 사용한 데스크톱 개발" 워크로드.
# CMake 는 VS 에 포함된 것을 자동으로 찾아 쓴다(별도 설치 불필요).
#
# 결과: out\tygis_dwg2dxf.exe

param([switch]$Clean)

$ErrorActionPreference = "Stop"
$root = $PSScriptRoot
$bld  = Join-Path $root "build"
$out  = Join-Path $root "out\tygis_dwg2dxf.exe"
$redwg = Join-Path $root "libredwg"

# ── libredwg 판 고정 ─────────────────────────────────────────
# **커밋을 박아 둔다.** GPL-3.0 은 배포한 바이너리에 «대응하는» 소스를
# 요구한다. 최신 master 를 받아 빌드하면 그 바이너리가 어느 소스로
# 만들어졌는지 특정할 수 없어 의무를 지킬 수 없다.
$LIBREDWG_COMMIT  = "d3a0a2dc1fdab5737bc6036db2d705300e6e59b6"
$LIBREDWG_VERSION = "0.14-gd3a0a2d"
$LIBREDWG_URL     = "https://github.com/LibreDWG/libredwg.git"

if (-not (Test-Path (Join-Path $redwg "CMakeLists.txt"))) {
    Write-Host "libredwg $LIBREDWG_COMMIT 를 받아옵니다..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Force $redwg | Out-Null
    git -C $redwg init -q
    git -C $redwg remote add origin $LIBREDWG_URL
    # 커밋을 콕 집어 얕게 받는다(GitHub 이 허용한다). 안 되면 통째로 받아
    # 그 커밋으로 되돌린다.
    git -C $redwg fetch -q --depth 1 origin $LIBREDWG_COMMIT
    if ($LASTEXITCODE -eq 0) {
        git -C $redwg checkout -q FETCH_HEAD
    } else {
        Write-Host "  얕은 받기 실패 — 전체를 받습니다" -ForegroundColor Yellow
        git -C $redwg fetch -q origin
        if ($LASTEXITCODE -ne 0) { throw "libredwg 를 받지 못했습니다" }
        git -C $redwg checkout -q $LIBREDWG_COMMIT
    }
    if ($LASTEXITCODE -ne 0) { throw "libredwg 커밋 $LIBREDWG_COMMIT 체크아웃 실패" }

    # 얕은 받기에는 태그가 없어 libredwg 의 CMake 가 git describe 로 판을
    # 못 구한다. .version 을 직접 써 두면 경고 없이 빌드된다.
    Set-Content -Path (Join-Path $redwg ".version") -Value $LIBREDWG_VERSION `
        -NoNewline -Encoding ascii
}

# 받아 둔 것이 고정한 커밋이 맞는지 확인한다 — 손으로 바꿔 두고 잊는 일이 있다.
$head = (git -C $redwg rev-parse HEAD).Trim()
if ($head -ne $LIBREDWG_COMMIT) {
    Write-Host "경고: libredwg 가 고정한 커밋이 아닙니다." -ForegroundColor Red
    Write-Host "      지금: $head" -ForegroundColor Red
    Write-Host "      기대: $LIBREDWG_COMMIT" -ForegroundColor Red
    Write-Host "      배포본을 만들 것이면 libredwg\ 를 지우고 다시 실행하세요." -ForegroundColor Red
}

# ── VS 에 포함된 CMake 찾기 ──────────────────────────────────
$vswhere = "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) { throw "Visual Studio 를 찾을 수 없습니다." }
$vsPath = & $vswhere -products * -latest -property installationPath
if (-not $vsPath) { throw "Visual Studio 설치를 찾을 수 없습니다." }

$cmake = Join-Path $vsPath "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if (-not (Test-Path $cmake)) {
    $cmakeCmd = Get-Command cmake -ErrorAction SilentlyContinue
    if ($cmakeCmd) { $cmake = $cmakeCmd.Source } else { throw "cmake 를 찾을 수 없습니다." }
}

$vsVer = & $vswhere -products * -latest -property installationVersion
$major = [int]($vsVer -split '\.')[0]
$generator = switch ($major) {
    18 { "Visual Studio 18 2026" }
    17 { "Visual Studio 17 2022" }
    default { throw "지원하지 않는 Visual Studio 버전: $vsVer" }
}
Write-Host "생성기: $generator" -ForegroundColor Cyan

if ($Clean -and (Test-Path $bld)) { Remove-Item $bld -Recurse -Force }
New-Item -ItemType Directory -Force $bld | Out-Null

Write-Host "Configure..." -ForegroundColor Cyan
& $cmake -S $root -B $bld -G $generator -A x64 | Out-Null
if ($LASTEXITCODE -ne 0) { throw "cmake configure 실패" }

Write-Host "Build..." -ForegroundColor Cyan
& $cmake --build $bld --config Release --target tygis_dwg2dxf --parallel |
    Select-String -Pattern "error C|error LNK|tygis_dwg2dxf\.exe"
if ($LASTEXITCODE -ne 0) { throw "cmake build 실패" }

$built = Join-Path $bld "Release\tygis_dwg2dxf.exe"
if (-not (Test-Path $built)) { throw "tygis_dwg2dxf.exe 가 생성되지 않았습니다." }

New-Item -ItemType Directory -Force (Split-Path $out) | Out-Null
Copy-Item $built $out -Force
$mb = [math]::Round((Get-Item $out).Length / 1MB, 2)
Write-Host "`n완료: $out ($mb MB)" -ForegroundColor Green
