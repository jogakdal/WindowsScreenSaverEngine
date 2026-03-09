#include "framework/Window.h"

namespace wsse {

Window::~Window() {
    if (hwnd_) {
        SetWindowLongPtrW(hwnd_, GWLP_USERDATA, 0);
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
    if (classRegistered_ && hInst_) {
        UnregisterClassW(className_, hInst_);
    }
}

bool Window::Create(HINSTANCE hInst, const wchar_t* className, const wchar_t* title,
                    DWORD style, DWORD exStyle,
                    int x, int y, int w, int h, HWND parent) {
    hInst_ = hInst;
    wcsncpy_s(className_, className, _TRUNCATE);

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProcThunk;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    wc.lpszClassName = className_;

    if (RegisterClassExW(&wc)) {
        classRegistered_ = true;
    }

    hwnd_ = CreateWindowExW(exStyle, className_, title, style,
                            x, y, w, h, parent, nullptr, hInst, this);
    return hwnd_ != nullptr;
}

void Window::Show(int cmdShow) {
    if (hwnd_) {
        ShowWindow(hwnd_, cmdShow);
        UpdateWindow(hwnd_);
    }
}

LRESULT Window::HandleMessage(UINT msg, WPARAM wp, LPARAM lp) {
    return DefWindowProcW(hwnd_, msg, wp, lp);
}

// WndProc 썽크: GWLP_USERDATA에 저장된 this 포인터로 가상 함수 호출
LRESULT CALLBACK Window::WndProcThunk(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    Window* self = nullptr;

    if (msg == WM_NCCREATE) {
        auto cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        self = static_cast<Window*>(cs->lpCreateParams);
        self->hwnd_ = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (self) {
        return self->HandleMessage(msg, wp, lp);
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

} // namespace wsse
