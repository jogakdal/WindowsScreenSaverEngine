#pragma once
#include "framework/Window.h"
#include <functional>
#include <vector>

namespace wsse {

// 모니터 정보
struct MonitorInfo {
    RECT rect;
    int width;
    int height;
};

// 전체화면 스크린세이버 윈도우
// 마우스/키보드 입력 시 종료, F1으로 설정 다이얼로그 열기
// 다중 모니터: 가장 큰 모니터에 프라이머리 창, 나머지에 미러 창 생성
// 에뮬레이션: /emu 또는 /emu:N으로 단일 모니터에서 다중 모니터 테스트
class ScreenSaverWindow : public Window {
public:
    using RenderCallback = std::function<void(HDC, int, int)>;

    ~ScreenSaverWindow();

    // emuMonitors > 0: 다중 모니터 에뮬레이션 (windowed 강제)
    bool Init(HINSTANCE hInst, bool windowed = false, int emuMonitors = 0);
    void SetRenderCallback(RenderCallback cb) { renderCallback_ = std::move(cb); }
    void RequestRedraw();

    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }

    bool IsConfigRequested() const { return configRequested_; }
    void ClearConfigRequest() { configRequested_ = false; }
    bool IsScreenshotRequested() const { return screenshotRequested_; }
    void ClearScreenshotRequest() { screenshotRequested_ = false; }
    void SetConfigDialogOpen(bool open) { configDialogOpen_ = open; }
    void ResetInputTracking();

    // 보조 모니터에 렌더 결과 복사 (종횡비 보존 중앙 크롭)
    void UpdateMirrorWindows(HDC sourceDC, int sourceW, int sourceH);
    int GetMonitorCount() const { return static_cast<int>(monitors_.size()); }

protected:
    LRESULT HandleMessage(UINT msg, WPARAM wp, LPARAM lp) override;

private:
    // 보조 창 마우스 추적 상태
    struct MirrorState {
        POINT initMousePos{};
        bool mouseInitialized = false;
        DWORD startTick = 0;
        bool emuMode = false;
    };

    void OnPaint();
    static std::vector<MonitorInfo> EnumerateMonitors();
    static std::vector<MonitorInfo> GenerateEmuMonitors(
        int count, int screenW, int screenH);
    void CreateMirrorWindows(HINSTANCE hInst);
    void DestroyMirrorWindows();
    static LRESULT CALLBACK MirrorWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

    RenderCallback renderCallback_;
    int width_ = 0;
    int height_ = 0;
    bool windowed_ = false;
    bool emuMode_ = false;
    POINT initMousePos_{};
    bool mouseInitialized_ = false;
    bool closing_ = false;
    bool configRequested_ = false;
    bool screenshotRequested_ = false;
    bool configDialogOpen_ = false;
    DWORD startTick_ = 0;
    static constexpr DWORD kGracePeriodMs = 2000;  // 시작 후 입력 무시 기간

    // 다중 모니터
    std::vector<MonitorInfo> monitors_;
    std::vector<HWND> mirrorHwnds_;
    std::vector<MirrorState> mirrorStates_;
    int primaryIndex_ = 0;
};

} // namespace wsse
