#include "framework/ScreenSaverWindow.h"
#include <windowsx.h>

namespace wsse {

ScreenSaverWindow::~ScreenSaverWindow() {
    DestroyMirrorWindows();
}

// ---- 모니터 열거 ----

std::vector<MonitorInfo> ScreenSaverWindow::EnumerateMonitors() {
    std::vector<MonitorInfo> result;
    EnumDisplayMonitors(nullptr, nullptr,
        [](HMONITOR, HDC, LPRECT lpRect, LPARAM data) -> BOOL {
            auto* monitors = reinterpret_cast<std::vector<MonitorInfo>*>(data);
            MonitorInfo info;
            info.rect = *lpRect;
            info.width = lpRect->right - lpRect->left;
            info.height = lpRect->bottom - lpRect->top;
            monitors->push_back(info);
            return TRUE;
        },
        reinterpret_cast<LPARAM>(&result));
    return result;
}

// ---- 에뮬레이션 모니터 생성 ----

std::vector<MonitorInfo> ScreenSaverWindow::GenerateEmuMonitors(
    int count, int screenW, int screenH)
{
    if (count < 2) count = 2;
    if (count > 4) count = 4;

    // 서로 다른 종횡비 프리셋 (크롭 로직 테스트용)
    static const float aspectRatios[] = {
        16.0f / 9.0f,   // 16:9
        4.0f / 3.0f,    // 4:3
        21.0f / 9.0f,   // 21:9 (울트라와이드)
        16.0f / 10.0f,  // 16:10
    };

    constexpr int gap = 16;
    int monH = screenH * 45 / 100;  // 화면 높이의 45%

    // 각 모니터 폭 계산
    std::vector<int> widths(count);
    int totalW = 0;
    for (int i = 0; i < count; i++) {
        widths[i] = static_cast<int>(monH * aspectRatios[i % 4]);
        totalW += widths[i];
    }
    totalW += (count - 1) * gap;

    // 화면에 맞게 스케일링
    int maxW = screenW - 40;  // 좌우 여백 20px
    if (totalW > maxW) {
        float scale = static_cast<float>(maxW) / totalW;
        monH = static_cast<int>(monH * scale);
        totalW = 0;
        for (int i = 0; i < count; i++) {
            widths[i] = static_cast<int>(monH * aspectRatios[i % 4]);
            totalW += widths[i];
        }
        totalW += (count - 1) * gap;
    }

    // 화면 중앙에 배치, 상단 정렬
    int startX = (screenW - totalW) / 2;
    int startY = (screenH - monH) / 2;

    std::vector<MonitorInfo> result;
    int x = startX;
    for (int i = 0; i < count; i++) {
        MonitorInfo info;
        info.rect = { x, startY, x + widths[i], startY + monH };
        info.width = widths[i];
        info.height = monH;
        result.push_back(info);
        x += widths[i] + gap;
    }

    return result;
}

// ---- 초기화 ----

bool ScreenSaverWindow::Init(HINSTANCE hInst, bool windowed, int emuMonitors) {
    startTick_ = GetTickCount();
    windowed_ = windowed;
    emuMode_ = (emuMonitors > 0);

    if (emuMode_) {
        // 에뮬레이션: windowed 강제, 가상 모니터 생성
        windowed_ = true;
        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);
        monitors_ = GenerateEmuMonitors(emuMonitors, screenW, screenH);
    } else {
        monitors_ = EnumerateMonitors();
        if (monitors_.empty()) {
            // 폴백: 단일 모니터
            int screenW = GetSystemMetrics(SM_CXSCREEN);
            int screenH = GetSystemMetrics(SM_CYSCREEN);
            monitors_.push_back({{0, 0, screenW, screenH}, screenW, screenH});
            primaryIndex_ = 0;
        }
    }

    // 가장 큰 모니터를 렌더 모니터로 선택 (면적 기준)
    int maxArea = 0;
    for (int i = 0; i < static_cast<int>(monitors_.size()); i++) {
        int area = monitors_[i].width * monitors_[i].height;
        if (area > maxArea) {
            maxArea = area;
            primaryIndex_ = i;
        }
    }

    auto& primary = monitors_[primaryIndex_];
    width_ = primary.width;
    height_ = primary.height;

    if (windowed_) {
        if (!Create(hInst, L"WSSEScreenSaverWnd", L"Screen Saver",
                    WS_POPUP | WS_VISIBLE, 0,
                    primary.rect.left, primary.rect.top, width_, height_))
            return false;
    } else {
        if (!Create(hInst, L"WSSEScreenSaverWnd", L"",
                    WS_POPUP | WS_VISIBLE,
                    WS_EX_TOPMOST,
                    primary.rect.left, primary.rect.top, width_, height_))
            return false;

        SetForegroundWindow(hwnd_);
        SetFocus(hwnd_);
    }

    // 보조 모니터에 미러 창 생성 (전체화면 또는 에뮬레이션, 2개 이상 모니터)
    if ((emuMode_ || !windowed_) && monitors_.size() > 1) {
        CreateMirrorWindows(hInst);
    }

    return true;
}

// ---- 보조(미러) 창 ----

void ScreenSaverWindow::CreateMirrorWindows(HINSTANCE hInst) {
    // 미러 창 클래스 등록
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = 0;
    wc.lpfnWndProc = MirrorWndProc;
    wc.hInstance = hInst;
    wc.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    wc.lpszClassName = L"WSSEMirrorWnd";
    RegisterClassExW(&wc);

    int mirrorCount = static_cast<int>(monitors_.size()) - 1;
    mirrorStates_.resize(mirrorCount);

    DWORD mirrorExStyle = emuMode_ ? 0 : WS_EX_TOPMOST;

    int stateIdx = 0;
    for (int i = 0; i < static_cast<int>(monitors_.size()); i++) {
        if (i == primaryIndex_) continue;

        auto& mon = monitors_[i];
        HWND mirror = CreateWindowExW(
            mirrorExStyle, L"WSSEMirrorWnd", L"",
            WS_POPUP | WS_VISIBLE,
            mon.rect.left, mon.rect.top, mon.width, mon.height,
            nullptr, nullptr, hInst, nullptr);

        if (mirror) {
            auto& st = mirrorStates_[stateIdx];
            st.startTick = startTick_;
            st.emuMode = emuMode_;
            st.width = mon.width;
            st.height = mon.height;

            // 백버퍼 생성 (WM_PAINT에서 사용)
            HDC screenDC = ::GetDC(mirror);
            st.backDC = CreateCompatibleDC(screenDC);
            st.backBmp = CreateCompatibleBitmap(screenDC, mon.width, mon.height);
            st.oldBmp = SelectObject(st.backDC, st.backBmp);
            // 초기 검정 배경
            RECT r = {0, 0, mon.width, mon.height};
            FillRect(st.backDC, &r, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
            ::ReleaseDC(mirror, screenDC);

            SetWindowLongPtrW(mirror, GWLP_USERDATA,
                              reinterpret_cast<LONG_PTR>(&st));
            mirrorHwnds_.push_back(mirror);
            stateIdx++;
        }
    }
}

void ScreenSaverWindow::DestroyMirrorWindows() {
    for (auto& st : mirrorStates_) {
        if (st.backDC) {
            SelectObject(st.backDC, st.oldBmp);
            DeleteDC(st.backDC);
        }
        if (st.backBmp)
            DeleteObject(st.backBmp);
    }
    for (HWND mirror : mirrorHwnds_) {
        if (mirror && IsWindow(mirror))
            DestroyWindow(mirror);
    }
    mirrorHwnds_.clear();
    mirrorStates_.clear();
}

LRESULT CALLBACK ScreenSaverWindow::MirrorWndProc(
    HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    auto* state = reinterpret_cast<MirrorState*>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        if (state && state->backDC) {
            BitBlt(hdc, 0, 0, state->width, state->height,
                   state->backDC, 0, 0, SRCCOPY);
        }
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (state && state->emuMode) {
            // 에뮬레이션: ESC만 종료
            if (wp == VK_ESCAPE) PostQuitMessage(0);
            return 0;
        }
        if (state && GetTickCount() - state->startTick < 2000) return 0;
        PostQuitMessage(0);
        return 0;

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
        if (state && state->emuMode) return 0;
        if (state && GetTickCount() - state->startTick < 2000) return 0;
        PostQuitMessage(0);
        return 0;

    case WM_MOUSEMOVE: {
        if (!state) return 0;
        if (state->emuMode) return 0;  // 에뮬레이션: 마우스 이동 무시
        if (GetTickCount() - state->startTick < 2000) return 0;
        POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        if (!state->mouseInitialized) {
            state->initMousePos = pt;
            state->mouseInitialized = true;
        } else {
            int dx = pt.x - state->initMousePos.x;
            int dy = pt.y - state->initMousePos.y;
            if (dx * dx + dy * dy > 25) {
                PostQuitMessage(0);
            }
        }
        return 0;
    }

    case WM_SETCURSOR:
        if (state && state->emuMode)
            return DefWindowProcW(hwnd, msg, wp, lp);  // 에뮬레이션: 커서 표시
        SetCursor(nullptr);
        return TRUE;

    default:
        return DefWindowProcW(hwnd, msg, wp, lp);
    }
}

// ---- 보조 창에 렌더 결과 복사 ----

void ScreenSaverWindow::UpdateMirrorWindows(
    HDC sourceDC, int sourceW, int sourceH)
{
    int mirrorIdx = 0;
    for (int i = 0; i < static_cast<int>(monitors_.size()); i++) {
        if (i == primaryIndex_) continue;
        if (mirrorIdx >= static_cast<int>(mirrorHwnds_.size())) break;

        int stateIdx = mirrorIdx;
        HWND mirror = mirrorHwnds_[mirrorIdx++];
        if (stateIdx >= static_cast<int>(mirrorStates_.size())) break;

        auto& st = mirrorStates_[stateIdx];
        if (!st.backDC) continue;

        auto& mon = monitors_[i];

        // 종횡비 보존 중앙 크롭
        float srcAspect = static_cast<float>(sourceW) / sourceH;
        float dstAspect = static_cast<float>(mon.width) / mon.height;

        int srcX = 0;
        int srcY = 0;
        int srcW = sourceW;
        int srcH = sourceH;

        if (srcAspect > dstAspect) {
            // 소스가 더 넓음 -> 좌우 크롭
            srcW = static_cast<int>(sourceH * dstAspect);
            srcX = (sourceW - srcW) / 2;
        } else if (srcAspect < dstAspect) {
            // 소스가 더 좁음 -> 상하 크롭
            srcH = static_cast<int>(sourceW / dstAspect);
            srcY = (sourceH - srcH) / 2;
        }

        // 백버퍼에 렌더링
        SetStretchBltMode(st.backDC, HALFTONE);
        StretchBlt(st.backDC, 0, 0, mon.width, mon.height,
                   sourceDC, srcX, srcY, srcW, srcH, SRCCOPY);

        // 즉시 화면 출력 (메시지 큐 대기 없이)
        HDC hdc = ::GetDC(mirror);
        if (hdc) {
            BitBlt(hdc, 0, 0, mon.width, mon.height,
                   st.backDC, 0, 0, SRCCOPY);
            ::ReleaseDC(mirror, hdc);
        }
        // 대기 중인 WM_PAINT 취소 (이미 최신 내용 출력 완료)
        ValidateRect(mirror, nullptr);
    }
}

// ---- 렌더 ----

void ScreenSaverWindow::RequestRedraw() {
    if (hwnd_) InvalidateRect(hwnd_, nullptr, FALSE);
}

void ScreenSaverWindow::OnPaint() {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd_, &ps);
    if (renderCallback_) {
        renderCallback_(hdc, width_, height_);
    }
    EndPaint(hwnd_, &ps);
}

// ---- 입력 추적 리셋 ----

void ScreenSaverWindow::ResetInputTracking() {
    mouseInitialized_ = false;
    startTick_ = GetTickCount();

    // 보조 창의 마우스 추적도 리셋
    for (auto& state : mirrorStates_) {
        state.mouseInitialized = false;
        state.startTick = GetTickCount();
    }
}

// ---- 메시지 처리 ----

LRESULT ScreenSaverWindow::HandleMessage(UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_PAINT:
        OnPaint();
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (configDialogOpen_) return 0;
        // F1: 설정 다이얼로그 열기 (스크린세이버 유지)
        if (wp == VK_F1 && !closing_) {
            configRequested_ = true;
            return 0;
        }
        // F2: 현재 화면 스크린샷 저장
        if (wp == VK_F2 && !closing_) {
            screenshotRequested_ = true;
            return 0;
        }
        // ESC: 모든 모드에서 종료
        if (wp == VK_ESCAPE && !closing_) {
            closing_ = true;
            PostQuitMessage(0);
        }
        // 전체화면: 아무 키나 누르면 종료
        if (!windowed_ && !closing_) {
            closing_ = true;
            PostQuitMessage(0);
        }
        return 0;

    case WM_MOUSEMOVE:
        if (interactiveMode_) return 0;
        if (windowed_ || configDialogOpen_) return 0;
        if (GetTickCount() - startTick_ < kGracePeriodMs) return 0;
        {
            POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
            if (!mouseInitialized_) {
                initMousePos_ = pt;
                mouseInitialized_ = true;
            } else {
                int dx = pt.x - initMousePos_.x;
                int dy = pt.y - initMousePos_.y;
                if (dx * dx + dy * dy > 25 && !closing_) {
                    closing_ = true;
                    PostQuitMessage(0);
                }
            }
        }
        return 0;

    case WM_LBUTTONDOWN:
        if (interactiveMode_) {
            clickX_ = GET_X_LPARAM(lp);
            clickY_ = GET_Y_LPARAM(lp);
            clickPending_ = true;
            return 0;
        }
        // 통상 모드: 종료
        if (windowed_ || configDialogOpen_) return 0;
        if (GetTickCount() - startTick_ < kGracePeriodMs) return 0;
        if (!closing_) {
            closing_ = true;
            PostQuitMessage(0);
        }
        return 0;
    case WM_RBUTTONDOWN:
        if (interactiveMode_) {
            exitRequested_ = true;
            return 0;
        }
        // fall through
    case WM_MBUTTONDOWN:
        if (windowed_ || configDialogOpen_) return 0;
        if (GetTickCount() - startTick_ < kGracePeriodMs) return 0;
        if (!closing_) {
            closing_ = true;
            PostQuitMessage(0);
        }
        return 0;

    case WM_ACTIVATE:
        if (windowed_ || configDialogOpen_) return 0;
        // 포커스 잃으면 종료 (다른 앱 활성화)
        if (LOWORD(wp) == WA_INACTIVE && !closing_) {
            if (GetTickCount() - startTick_ < kGracePeriodMs) return 0;
            // 같은 프로세스의 창(보조 미러 창)에 의한 비활성화는 무시
            HWND activatedWnd = reinterpret_cast<HWND>(lp);
            if (activatedWnd) {
                DWORD pid = 0;
                GetWindowThreadProcessId(activatedWnd, &pid);
                if (pid == GetCurrentProcessId()) return 0;
            }
            closing_ = true;
            PostQuitMessage(0);
        }
        return 0;

    case WM_SETCURSOR:
        if (windowed_ || configDialogOpen_ || interactiveMode_)
            return DefWindowProcW(hwnd_, msg, wp, lp);
        SetCursor(nullptr);
        return TRUE;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        return Window::HandleMessage(msg, wp, lp);
    }
}

} // namespace wsse
