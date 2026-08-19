# tygis_dwg2dxf

DWG 파일을 DXF 로 바꾸는 **독립 실행 프로그램**입니다.

```
tygis_dwg2dxf.exe <입력.dwg> <출력.dxf> [--r12]
```

| 인자 | 뜻 |
|---|---|
| `--r12` | **R12(AC1009)** 로 낮춰 씁니다. 없으면 원본 DWG 판 그대로 씁니다. |

- 성공하면 종료코드 `0` 과 stdout 에 `OK <DWG버전>`
- 실패하면 0 이 아닌 코드와 stderr 에 `ERR code=<n> bits=0x<hex>`
  (코드 값은 [`dwgbridge/dwgbridge.h`](dwgbridge/dwgbridge.h) 참조)

## `--r12` 를 언제 쓰나

**AutoCAD 로 열 DXF 라면 `--r12` 를 쓰십시오.**

원본 판 그대로 쓴 DXF 에는 `CLASSES`·`OBJECTS` 섹션이 붙는데, AutoCAD Map 3D
오브젝트 데이터(`ADE…` XRECORD)가 많이 든 도면에서는 그 부분이 AutoCAD 가
받아들이는 형태로 나오지 않아 **파일이 열리지 않습니다.** R12 는 두 섹션이
아예 없어 훨씬 잘 열립니다.

대신 R12 는 트루컬러·선가중치와 `LWPOLYLINE`(→`POLYLINE`) 같은 최신 표현을
잃습니다. **프로그램이 읽어 화면에 그릴 목적이라면 원본 판 그대로**가 낫습니다.

## 라이선스 (중요)

이 프로그램은 **[GNU LibreDWG](https://www.gnu.org/software/libredwg/)** 를
정적 링크합니다. LibreDWG 가 **GPL-3.0-or-later** 이므로 **이 프로그램 전체가
GPL-3.0 으로 배포**됩니다. 전문은 [`LICENSE`](LICENSE) 를 보십시오.

### 왜 별도 프로그램인가

GPL-3.0 은 강한 카피레프트라, GPL 코드를 링크한 프로그램을 배포하면 그
프로그램 **전체**의 소스를 GPL 로 공개해야 합니다.

이것을 쓰는 프로그램은 이 실행파일을 **별개 프로세스로 실행**하고 결과를
**DXF 라는 표준 파일 포맷으로만** 주고받습니다. 링크하지 않으므로(arm's-length)
호출하는 쪽은 GPL 결합저작물이 아니며, 이 변환기만 GPL 로 배포합니다.

> ⚠️ **libredwg 를 호출자 프로세스에 직접 링크(FFI·정적/동적 링크)하지 마십시오.**
> 그 순간 그 프로그램 전체가 GPL 대상이 됩니다.

### libredwg 판 고정

[`build.ps1`](build.ps1) 의 `$LIBREDWG_COMMIT` 이 **어느 libredwg 로 빌드하는지**
정하는 정본입니다.

```
d3a0a2dc1fdab5737bc6036db2d705300e6e59b6   (0.14-gd3a0a2d, 2026-07-29)
```

최신 master 를 받아 빌드하면 배포한 바이너리에 **«대응하는» 소스**를 특정할 수
없어 GPL 의무를 지킬 수 없습니다. 그래서 커밋을 박아 두고, 빌드할 때마다 받아 둔
것이 그 커밋인지 확인합니다. **판을 올릴 때는 이 값을 바꾸고 함께 커밋하십시오.**

## 빌드

Visual Studio 2022 이상 + "C++를 사용한 데스크톱 개발" 워크로드가 필요합니다.
CMake 는 VS 에 포함된 것을 자동으로 찾습니다.

```powershell
powershell -ExecutionPolicy Bypass -File build.ps1
```

libredwg 는 위 커밋으로 자동으로 받아옵니다(`libredwg/`, 저장소에는 담지 않음).
결과는 `out\tygis_dwg2dxf.exe` (약 7.5MB).

## 구조

```
CMakeLists.txt        libredwg 를 라이브러리로만 빌드해 정적 링크
build.ps1             libredwg 받아오기(커밋 고정) + 빌드
dwgbridge/
├─ dwgbridge.h        얇은 C API 선언 (반환코드 정의)
├─ dwgbridge.c        libredwg 호출 래퍼 (UTF-8 → ANSI 경로 변환 포함)
└─ dwg2dxf_main.c     실행파일 진입점
```

### 알아둘 것

- **`main()` 이 아니라 `wmain()` 을 씁니다.** Windows 의 `main` 은 인자를 활성
  ANSI 코드페이지로 주는데, 한글 경로를 안전히 넘기려면 UTF-16 → UTF-8 변환이
  필요합니다. `main()` 을 쓰면 한글 경로가 `code=5` 로 실패합니다.
- **MSVC 에서 `CHECK_INCLUDE_FILE("wchar.h")` 가 간헐적으로 실패합니다.**
  실패하면 libredwg 의 `codepages.h` 가 `wchar_t` 를 재정의해 `C2371` 로
  빌드가 깨집니다. `CMakeLists.txt` 에서 검사 결과를 미리 고정해 두었습니다.
