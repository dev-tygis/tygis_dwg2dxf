/*
 * tygis_dwg2dxf — DWG → DXF 변환 독립 실행파일.
 *
 *   tygis_dwg2dxf <입력.dwg> <출력.dxf>
 *   종료코드: 0 성공, 그 외 dwgbridge.h 의 오류코드
 *
 * 이 프로그램은 LibreDWG(GPL-3.0)를 정적 링크한다. 본 프로그램 전체는
 * GPL-3.0 으로 배포된다(native/LICENSE.GPL 참조). 상위 앱(TY GIS Studio)은
 * 이 프로그램을 별개 프로세스로 실행할 뿐이라 결합저작물이 아니며,
 * 독립적으로 배포된다.
 */
#define DWGBRIDGE_BUILDING 1
#include "dwgbridge.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#  include <windows.h>

/* Windows 의 main(argc, argv) 는 인자를 활성 ANSI 코드페이지로 준다.
   dwgbridge_dwg_to_dxf() 는 UTF-8 을 받으므로 wmain 으로 UTF-16 을 받아
   UTF-8 로 바꿔 넘긴다. 이렇게 해야 한글 경로가 그대로 처리된다. */
static char *wide_to_utf8(const wchar_t *w)
{
  const int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, NULL, 0, NULL, NULL);
  if (n <= 0)
    return NULL;
  char *out = (char *)malloc((size_t)n);
  if (!out)
    return NULL;
  if (WideCharToMultiByte(CP_UTF8, 0, w, -1, out, n, NULL, NULL) <= 0)
    {
      free(out);
      return NULL;
    }
  return out;
}

int wmain(int argc, wchar_t **argv)
{
  if (argc < 3)
    {
      fwprintf(stderr, L"usage: tygis_dwg2dxf <in.dwg> <out.dxf> [--r12]\n");
      return DWGBRIDGE_ERR_ARGS;
    }

  char *in = wide_to_utf8(argv[1]);
  char *out = wide_to_utf8(argv[2]);
  if (!in || !out)
    {
      free(in);
      free(out);
      fprintf(stderr, "ERR code=%d (경로 인코딩 변환 실패)\n", DWGBRIDGE_ERR_PATH);
      return DWGBRIDGE_ERR_PATH;
    }

  /* --r12 : CLASSES·OBJECTS 없는 R12 DXF 로 낮춰 쓴다(호환성 우선). */
  int as_r12 = 0;
  for (int i = 3; i < argc; i++)
    if (wcscmp(argv[i], L"--r12") == 0)
      as_r12 = 1;

  const int rc = dwgbridge_dwg_to_dxf_as(in, out, as_r12);
  free(in);
  free(out);

  if (rc == DWGBRIDGE_OK)
    {
      /* 상위 앱이 stdout 한 줄로 성공/버전을 읽는다. */
      fprintf(stdout, "OK %s\n", dwgbridge_last_version());
      fflush(stdout);
      return DWGBRIDGE_OK;
    }

  fprintf(stderr, "ERR code=%d bits=0x%x\n", rc, dwgbridge_last_error_bits());
  return rc;
}

#else /* !_WIN32 — POSIX 는 argv 가 이미 UTF-8 */

int main(int argc, char **argv)
{
  if (argc < 3)
    {
      fprintf(stderr, "usage: tygis_dwg2dxf <in.dwg> <out.dxf> [--r12]\n");
      return DWGBRIDGE_ERR_ARGS;
    }

  int as_r12 = 0;
  for (int i = 3; i < argc; i++)
    if (strcmp(argv[i], "--r12") == 0)
      as_r12 = 1;

  const int rc = dwgbridge_dwg_to_dxf_as(argv[1], argv[2], as_r12);
  if (rc == DWGBRIDGE_OK)
    {
      fprintf(stdout, "OK %s\n", dwgbridge_last_version());
      fflush(stdout);
      return DWGBRIDGE_OK;
    }

  fprintf(stderr, "ERR code=%d bits=0x%x\n", rc, dwgbridge_last_error_bits());
  return rc;
}

#endif
