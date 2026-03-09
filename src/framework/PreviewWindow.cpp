#include "framework/PreviewWindow.h"

namespace wsse {

bool PreviewWindow::Init(HINSTANCE hInst, HWND parentHwnd) {
    RECT rc;
    GetClientRect(parentHwnd, &rc);
    width_ = rc.right - rc.left;
    height_ = rc.bottom - rc.top;

    // 미리보기 패널 내부의 자식 윈도우
    return Create(hInst, L"WSSEPreviewWnd", L"",
                  WS_CHILD | WS_VISIBLE,
                  0,
                  0, 0, width_, height_,
                  parentHwnd);
}

void PreviewWindow::RequestRedraw() {
    if (hwnd_) InvalidateRect(hwnd_, nullptr, FALSE);
}

LRESULT PreviewWindow::HandleMessage(UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd_, &ps);
        if (renderCallback_) {
            renderCallback_(hdc, width_, height_);
        }
        EndPaint(hwnd_, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return Window::HandleMessage(msg, wp, lp);
    }
}

} // namespace wsse
