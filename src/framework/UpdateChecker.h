#pragma once
#include <windows.h>

namespace wsse {

// 업데이트 확인 결과를 전달하는 커스텀 메시지
constexpr UINT WM_UPDATE_RESULT = WM_APP + 100;

// wParam 값
constexpr WPARAM UPDATE_NONE = 0;       // 최신 버전
constexpr WPARAM UPDATE_AVAILABLE = 1;  // 새 버전 있음
constexpr WPARAM UPDATE_ERROR = 2;      // 확인 실패

// 업데이트 정보 (lParam으로 전달, 수신측에서 delete)
struct UpdateInfo {
    wchar_t version[32];
    wchar_t url[512];
};

// 백그라운드 스레드에서 GitHub Releases API 호출,
// 완료 시 hwnd에 WM_UPDATE_RESULT 메시지 전달
//
// host: API 호스트 (e.g., "api.github.com")
// path: API 경로 (e.g., "/repos/user/repo/releases/latest")
// userAgent: HTTP User-Agent (e.g., "MyApp/1.0")
void CheckForUpdateAsync(HWND hwnd,
                         const wchar_t* host,
                         const wchar_t* path,
                         const wchar_t* userAgent);

} // namespace wsse
