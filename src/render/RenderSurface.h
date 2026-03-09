#pragma once
#include <windows.h>
#include <cstdint>

namespace wsse {

// DIB 섹션 래퍼: 픽셀 버퍼 렌더링용
class RenderSurface {
public:
    ~RenderSurface();

    bool Create(int width, int height);
    void BlitTo(HDC destDC, int destX, int destY) const;
    void StretchBlitTo(HDC destDC, int destX, int destY, int destW, int destH) const;
    // GDI DPI 가상화 우회 직접 픽셀 복사
    void DirectBlitTo(HDC destDC) const;
    // 현재 프레임을 BMP 파일로 저장
    bool SaveBMP(const wchar_t* path) const;
    // 현재 프레임을 PNG 파일로 저장 (GDI+)
    bool SavePNG(const wchar_t* path) const;

    void SetPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);
    void SetPixelBGRA(int x, int y, uint32_t bgra);
    void Clear(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0);

    uint32_t* GetPixels() { return pixels_; }
    const uint32_t* GetPixels() const { return pixels_; }
    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }
    int GetStride() const { return width_; } // uint32_t 단위

    HDC GetMemDC() const { return memDC_; }

private:
    HDC memDC_ = nullptr;
    HBITMAP bitmap_ = nullptr;
    HBITMAP oldBitmap_ = nullptr;
    uint32_t* pixels_ = nullptr;
    int width_ = 0;
    int height_ = 0;
};

} // namespace wsse
