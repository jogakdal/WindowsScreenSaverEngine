#pragma once
#include <windows.h>

namespace wsse {

// Win32 윈도우 기본 래퍼 클래스
// WndProc 썽크로 C++ 가상 함수 디스패치 지원
class Window {
public:
    virtual ~Window();

    bool Create(HINSTANCE hInst, const wchar_t* className, const wchar_t* title,
                DWORD style, DWORD exStyle,
                int x, int y, int w, int h,
                HWND parent = nullptr);

    void Show(int cmdShow = SW_SHOW);
    HWND GetHwnd() const { return hwnd_; }

protected:
    Window() = default;
    virtual LRESULT HandleMessage(UINT msg, WPARAM wp, LPARAM lp);

    HWND hwnd_ = nullptr;
    HINSTANCE hInst_ = nullptr;

private:
    static LRESULT CALLBACK WndProcThunk(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    bool classRegistered_ = false;
    wchar_t className_[64]{};
};

} // namespace wsse
