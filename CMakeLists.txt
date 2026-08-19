# tygis_dwg2dxf — libredwg(GPL-3.0)를 정적 링크한 DWG→DXF 변환 실행파일.
#
# 빌드:  build.ps1   (libredwg 는 고정한 커밋으로 자동으로 받아온다)
#
# 이 프로그램 전체가 GPL-3.0 으로 배포된다 — LICENSE 참조.
# 이것을 쓰는 프로그램은 **별개 프로세스로 실행**할 뿐 링크하지 않으므로
# (arm's-length) GPL 결합저작물이 아니다. 그 경계를 지키려면 libredwg 를
# 호출자 프로세스에 직접 링크(FFI·정적/동적 링크)해서는 안 된다.
cmake_minimum_required(VERSION 3.20)
project(tygis_dwg2dxf C)

set(CMAKE_C_STANDARD 99)

# libredwg: 라이브러리만, 쓰기/JSON 등 불필요한 부분 제외해 크기를 줄인다.
set(LIBREDWG_LIBONLY On CACHE BOOL "" FORCE)
set(LIBREDWG_DISABLE_JSON On CACHE BOOL "" FORCE)
set(BUILD_TESTING Off CACHE BOOL "" FORCE)

if(MSVC)
  # libredwg 의 CHECK_INCLUDE_FILE("wchar.h") 가 MSVC 에서 간헐적으로 실패한다.
  # 실패하면 src/codepages.h 가 `typedef uint32_t wchar_t` 로 내려가
  # MSVC 내장 wchar_t 와 충돌해 빌드가 깨진다(C2371).
  # MSVC 에는 항상 wchar.h 가 있으므로 검사 결과를 미리 고정한다.
  set(HAVE_WCHAR_H 1 CACHE INTERNAL "wchar.h — MSVC 는 항상 제공")
endif()

add_subdirectory(libredwg EXCLUDE_FROM_ALL)

# MSVC 는 타깃 이름이 libredwg, 그 외는 redwg
if(MSVC)
  set(REDWG_TARGET libredwg)
else()
  set(REDWG_TARGET redwg)
endif()

# libredwg 의 PACKAGE_VERSION 은 상대 경로로 .version 을 찾기 때문에
# 상위 프로젝트에서 add_subdirectory 로 부르면 비어버린다. 직접 읽어 넘긴다.
set(_libredwg_version_file ${CMAKE_CURRENT_SOURCE_DIR}/libredwg/.version)
if(EXISTS ${_libredwg_version_file})
  file(READ ${_libredwg_version_file} _libredwg_version)
  string(STRIP "${_libredwg_version}" _libredwg_version)
else()
  set(_libredwg_version "unknown")
endif()
message(STATUS "libredwg version: ${_libredwg_version}")

add_executable(tygis_dwg2dxf
  dwgbridge/dwg2dxf_main.c
  dwgbridge/dwgbridge.c
)

target_compile_definitions(tygis_dwg2dxf PRIVATE
  DWGBRIDGE_LIBREDWG_VERSION="${_libredwg_version}"
)

target_include_directories(tygis_dwg2dxf PRIVATE
  dwgbridge
  libredwg/include
  libredwg/src
  ${CMAKE_CURRENT_BINARY_DIR}/libredwg/src   # 생성된 config.h
)

target_link_libraries(tygis_dwg2dxf PRIVATE ${REDWG_TARGET})

if(MSVC)
  target_compile_definitions(tygis_dwg2dxf PRIVATE
    _CRT_SECURE_NO_WARNINGS
    _CRT_NONSTDC_NO_WARNINGS
  )
  # libredwg 헤더가 유발하는 과한 경고 억제
  target_compile_options(tygis_dwg2dxf PRIVATE /wd4244 /wd4267 /wd4996)
endif()
