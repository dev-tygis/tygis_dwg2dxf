#define DWGBRIDGE_BUILDING 1
#include "dwgbridge.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#  include <windows.h>
#endif

#include "dwg.h"
#include "bits.h"
#include "out_dxf.h"

static unsigned int g_last_bits = 0;
static char g_last_version[32] = { 0 };

DWGBRIDGE_API unsigned int
dwgbridge_last_error_bits (void)
{
  return g_last_bits;
}

DWGBRIDGE_API const char *
dwgbridge_last_version (void)
{
  return g_last_version;
}

/* 빌드 시 CMake 가 주입한다 (native/CMakeLists.txt). */
#ifndef DWGBRIDGE_LIBREDWG_VERSION
#  define DWGBRIDGE_LIBREDWG_VERSION "unknown"
#endif

DWGBRIDGE_API const char *
dwgbridge_version (void)
{
  return "dwgbridge 1.0 / libredwg " DWGBRIDGE_LIBREDWG_VERSION;
}

#ifdef _WIN32
/* libredwg 는 내부에서 fopen(const char*) 을 쓴다. Windows 에서 그 경로는
   UTF-8 이 아니라 활성 ANSI 코드페이지로 해석되므로, UTF-8 입력을 ANSI 로
   변환해 넘긴다. 한국어 Windows(CP949)에서는 한글 경로가 정상 변환된다.

   변환에서 문자가 손실되면(예: CP949 에 없는 문자) 실패를 알려 호출자가
   ASCII 임시 경로로 복사해 재시도하도록 한다. */
static int
utf8_to_acp (const char *utf8, char *out, int out_size)
{
  int wlen = MultiByteToWideChar (CP_UTF8, 0, utf8, -1, NULL, 0);
  if (wlen <= 0)
    return 0;

  wchar_t *w = (wchar_t *)malloc ((size_t)wlen * sizeof (wchar_t));
  if (!w)
    return 0;
  if (MultiByteToWideChar (CP_UTF8, 0, utf8, -1, w, wlen) <= 0)
    {
      free (w);
      return 0;
    }

  BOOL lossy = FALSE;
  int n = WideCharToMultiByte (CP_ACP, WC_NO_BEST_FIT_CHARS, w, -1, out,
                               out_size, NULL, &lossy);
  free (w);
  if (n <= 0 || lossy)
    return 0;
  return 1;
}
#endif

DWGBRIDGE_API int
dwgbridge_dwg_to_dxf (const char *in_path, const char *out_path)
{
  return dwgbridge_dwg_to_dxf_as (in_path, out_path, 0);
}

DWGBRIDGE_API int
dwgbridge_dwg_to_dxf_as (const char *in_path, const char *out_path,
                         int as_r12)
{
  g_last_bits = 0;
  g_last_version[0] = '\0';

  if (!in_path || !out_path || !*in_path || !*out_path)
    return DWGBRIDGE_ERR_ARGS;

#ifdef _WIN32
  char in_acp[1024];
  char out_acp[1024];
  if (!utf8_to_acp (in_path, in_acp, (int)sizeof (in_acp))
      || !utf8_to_acp (out_path, out_acp, (int)sizeof (out_acp)))
    return DWGBRIDGE_ERR_PATH;
  const char *in_native = in_acp;
  const char *out_native = out_acp;
#else
  const char *in_native = in_path;
  const char *out_native = out_path;
#endif

  Dwg_Data dwg;
  memset (&dwg, 0, sizeof (Dwg_Data));
  dwg.opts = 0; /* 로그 출력 없음 */

  int error = dwg_read_file (in_native, &dwg);
  g_last_bits = (unsigned int)error;
  if (error >= DWG_ERR_CRITICAL)
    {
      dwg_free (&dwg);
      return DWGBRIDGE_ERR_READ;
    }

  const char *v = dwg_version_type (dwg.header.version);
  if (v)
    {
      strncpy (g_last_version, v, sizeof (g_last_version) - 1);
      g_last_version[sizeof (g_last_version) - 1] = '\0';
    }

  Bit_Chain dat;
  memset (&dat, 0, sizeof (Bit_Chain));
  /* R12 로 낮춰 쓰면 CLASSES·OBJECTS 가 통째로 빠져 호환성이 올라간다.
     libredwg 의 dwg2dxf --as r12 와 같은 방식으로 헤더 판까지 바꾼다. */
  if (as_r12)
    dwg.header.version = R_12;

  dat.version = dwg.header.version;
  dat.from_version = dwg.header.from_version;

  dat.fh = fopen (out_native, "wb");
  if (!dat.fh)
    {
      dwg_free (&dwg);
      return DWGBRIDGE_ERR_WRITE;
    }

  error = dwg_write_dxf (&dat, &dwg);
  g_last_bits |= (unsigned int)error;

  fclose (dat.fh);
  dwg_free (&dwg);

  if (error >= DWG_ERR_CRITICAL)
    return DWGBRIDGE_ERR_UNSUPPORTED;
  return DWGBRIDGE_OK;
}
