// dwgbridge — DWG → DXF 변환만 노출하는 얇은 C API.
//
// libredwg 를 정적으로 링크하고, dart:ffi 에서 쓰기 쉬운 최소 표면만 내보낸다.
// 구조체를 넘기지 않으므로 Dart 쪽 바인딩이 단순하고 ABI 가 안정적이다.
#ifndef DWGBRIDGE_H_
#define DWGBRIDGE_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
#  ifdef DWGBRIDGE_BUILDING
#    define DWGBRIDGE_API __declspec(dllexport)
#  else
#    define DWGBRIDGE_API __declspec(dllimport)
#  endif
#else
#  define DWGBRIDGE_API __attribute__((visibility("default")))
#endif

// 반환 코드
#define DWGBRIDGE_OK              0
#define DWGBRIDGE_ERR_ARGS        1
#define DWGBRIDGE_ERR_READ        2  // DWG 를 읽지 못함 (버전 미지원/손상)
#define DWGBRIDGE_ERR_WRITE       3  // 출력 파일 생성/쓰기 실패
#define DWGBRIDGE_ERR_UNSUPPORTED 4  // 읽었으나 치명적 오류
// 경로를 시스템 코드페이지로 표현할 수 없음.
// libredwg 가 내부에서 fopen(const char*) 을 쓰기 때문에 생기는 제약이다.
// 호출자는 ASCII 임시 경로로 복사한 뒤 재시도하면 된다.
#define DWGBRIDGE_ERR_PATH        5

// DWG 파일을 DXF 로 변환한다.
//
//   in_path   UTF-8 경로 (Windows 에서는 내부적으로 UTF-16 으로 변환해 연다)
//   out_path  UTF-8 경로. 이미 있으면 덮어쓴다.
//
// 반환: DWGBRIDGE_* 코드. 0 이면 성공.
DWGBRIDGE_API int dwgbridge_dwg_to_dxf(const char *in_path,
                                       const char *out_path);

// 위와 같되 **DXF 판을 골라 쓴다.**
//
//   as_r12  0 이면 원본 DWG 판 그대로, 1 이면 R12 로 낮춰 쓴다.
//
// R12 DXF 는 CLASSES·OBJECTS 섹션과 핸들이 아예 없어 **다른 CAD 가 훨씬
// 잘 받는다.** 대신 트루컬러·선가중치·최신 엔티티 속성 일부를 잃는다.
// 원본 판 그대로 쓴 DXF 를 AutoCAD 가 거부할 때 쓰는 길이다.
DWGBRIDGE_API int dwgbridge_dwg_to_dxf_as(const char *in_path,
                                          const char *out_path,
                                          int as_r12);

// 마지막 호출에서 libredwg 가 돌려준 원시 오류 비트마스크.
// 0 이 아니어도 변환은 성공했을 수 있다(경고 수준).
DWGBRIDGE_API unsigned int dwgbridge_last_error_bits(void);

// 읽어들인 도면의 DWG 버전 문자열(예 "AC1015"). 실패 시 빈 문자열.
// 반환 포인터는 라이브러리가 소유하며 다음 호출까지 유효하다.
DWGBRIDGE_API const char *dwgbridge_last_version(void);

// 라이브러리 버전 (진단용)
DWGBRIDGE_API const char *dwgbridge_version(void);

#ifdef __cplusplus
}
#endif

#endif  // DWGBRIDGE_H_
