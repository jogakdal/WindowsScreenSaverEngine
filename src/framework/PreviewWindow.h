#pragma once
#include "framework/Window.h"
#include <functional>

namespace wsse {

// 제어판 미리보기 윈도우 (부모 HWND의 자식 윈도우)
class PreviewWindow : public Window {
public:
    using RenderCallback = std::function<void(HDC, int, int)>;

    bool Init(HINSTANCE hInst, HWND parentHwnd);
    void SetRenderCallback(RenderCallback cb) { renderCallback_ = std::move(cb); }
    void RequestRedraw();

    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }

protected:
    LRESULT HandleMessage(UINT msg, WPARAM wp, LPARAM lp) override;

private:
    RenderCallback renderCallback_;
    int width_ = 0;
    int height_ = 0;
};

} // namespace wsse
