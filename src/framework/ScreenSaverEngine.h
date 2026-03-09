#pragma once
#include "framework/AppDescriptor.h"
#include "framework/FrameworkSettings.h"
#include "framework/IScreenSaverContent.h"
#include <memory>

namespace wsse {

// 앱 실행 모드
enum class AppMode {
    ScreenSaver,  // /s - 전체화면 스크린세이버
    Configure,    // /c - 설정 다이얼로그
    Preview       // /p <hwnd> - 제어판 미리보기
};

// 스크린샷 설정
struct ScreenshotConfig {
    int intervalSec = 0;              // 0 = 비활성
    wchar_t folder[MAX_PATH] = {};    // 저장 폴더
};

// 스크린세이버 엔진
//
// AppDescriptor와 IScreenSaverContent를 받아 스크린세이버의 전체 라이프사이클을 관리합니다.
// 커맨드라인 파싱, 윈도우 관리, 메시지 루프, 프레임 타이밍, 오버레이/시계 합성,
// 시간 보간, 스크린샷, 설치/제거, 자동 업데이트를 처리합니다.
//
// 사용 예:
//   auto desc = MyContent::GetDescriptor();
//   auto content = std::make_unique<MyContent>();
//   ScreenSaverEngine engine(desc, std::move(content));
//   return engine.Run(hInstance);
class ScreenSaverEngine {
public:
    ScreenSaverEngine(const AppDescriptor& desc,
                      std::unique_ptr<IScreenSaverContent> content);
    ~ScreenSaverEngine();

    // 앱 실행 (커맨드라인 자동 파싱, 모드별 분기)
    int Run(HINSTANCE hInst);

private:
    // 모드별 실행
    int RunScreenSaver(HINSTANCE hInst, const ScreenshotConfig* screenshot);
    int RunConfigure(HINSTANCE hInst);
    int RunPreview(HINSTANCE hInst, HWND parent);

    // 설치/제거
    int DoInstall();
    int DoUninstall();
    int ShowLauncher(HINSTANCE hInst);

    // 유틸리티
    static bool IsElevated();
    static bool RelaunchElevated(const wchar_t* args);
    int ElevateIfNeeded(const wchar_t* args);

    AppDescriptor desc_;
    std::unique_ptr<IScreenSaverContent> content_;
    FrameworkSettings settings_;
    bool silent_ = false;
    int emuMonitors_ = 0;  // 다중 모니터 에뮬레이션 (0=비활성, 2~4=모니터 수)
};

} // namespace wsse
