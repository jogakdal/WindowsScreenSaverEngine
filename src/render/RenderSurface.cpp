#include "render/RenderSurface.h"
#include <cstring>
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

namespace wsse {

RenderSurface::~RenderSurface() {
    if (memDC_) {
        if (oldBitmap_) SelectObject(memDC_, oldBitmap_);
        DeleteDC(memDC_);
    }
    if (bitmap_) DeleteObject(bitmap_);
}

bool RenderSurface::Create(int width, int height) {
    width_ = width;
    height_ = height;

    // 실제 화면 해상도 기준 DC 생성 (DPI 인식 모드)
    HDC screenDC = ::GetDC(nullptr);
    memDC_ = CreateCompatibleDC(screenDC);
    ReleaseDC(nullptr, screenDC);

    // 탑다운 32비트 DIB 섹션
    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    bitmap_ = CreateDIBSection(memDC_, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!bitmap_) return false;

    pixels_ = static_cast<uint32_t*>(bits);
    oldBitmap_ = static_cast<HBITMAP>(SelectObject(memDC_, bitmap_));

    Clear();
    return true;
}

void RenderSurface::BlitTo(HDC destDC, int destX, int destY) const {
    BitBlt(destDC, destX, destY, width_, height_, memDC_, 0, 0, SRCCOPY);
}

void RenderSurface::StretchBlitTo(HDC destDC, int destX, int destY, int destW, int destH) const {
    SetStretchBltMode(destDC, HALFTONE);
    StretchBlt(destDC, destX, destY, destW, destH,
               memDC_, 0, 0, width_, height_, SRCCOPY);
}

void RenderSurface::DirectBlitTo(HDC destDC) const {
    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width_;
    bmi.bmiHeader.biHeight = -height_;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    SetDIBitsToDevice(destDC,
        0, 0, width_, height_,
        0, 0,
        0, height_,
        pixels_, &bmi, DIB_RGB_COLORS);
}

void RenderSurface::SetPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    if (x >= 0 && x < width_ && y >= 0 && y < height_) {
        pixels_[y * width_ + x] = (0xFF << 24) | (r << 16) | (g << 8) | b;
    }
}

void RenderSurface::SetPixelBGRA(int x, int y, uint32_t bgra) {
    if (x >= 0 && x < width_ && y >= 0 && y < height_) {
        pixels_[y * width_ + x] = bgra;
    }
}

void RenderSurface::Clear(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t color = (0xFF << 24) | (r << 16) | (g << 8) | b;
    int total = width_ * height_;
    for (int i = 0; i < total; i++) {
        pixels_[i] = color;
    }
}

bool RenderSurface::SaveBMP(const wchar_t* path) const {
    if (!pixels_ || width_ <= 0 || height_ <= 0) return false;

    BITMAPFILEHEADER fh{};
    BITMAPINFOHEADER ih{};
    int rowBytes = width_ * 4;
    int imageSize = rowBytes * height_;

    fh.bfType = 0x4D42; // 'BM'
    fh.bfSize = sizeof(fh) + sizeof(ih) + imageSize;
    fh.bfOffBits = sizeof(fh) + sizeof(ih);

    ih.biSize = sizeof(ih);
    ih.biWidth = width_;
    ih.biHeight = -height_;
    ih.biPlanes = 1;
    ih.biBitCount = 32;
    ih.biCompression = BI_RGB;
    ih.biSizeImage = imageSize;

    HANDLE hFile = CreateFileW(path, GENERIC_WRITE, 0, nullptr,
                                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    DWORD written;
    WriteFile(hFile, &fh, sizeof(fh), &written, nullptr);
    WriteFile(hFile, &ih, sizeof(ih), &written, nullptr);
    WriteFile(hFile, pixels_, imageSize, &written, nullptr);
    CloseHandle(hFile);
    return true;
}

// PNG 인코더 CLSID 검색
static bool GetPngEncoderClsid(CLSID* clsid) {
    UINT num = 0;
    UINT size = 0;
    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0) return false;

    auto* info = static_cast<Gdiplus::ImageCodecInfo*>(malloc(size));
    if (!info) return false;

    Gdiplus::GetImageEncoders(num, size, info);
    bool found = false;
    for (UINT i = 0; i < num; i++) {
        if (wcscmp(info[i].MimeType, L"image/png") == 0) {
            *clsid = info[i].Clsid;
            found = true;
            break;
        }
    }
    free(info);
    return found;
}

bool RenderSurface::SavePNG(const wchar_t* path) const {
    if (!pixels_ || width_ <= 0 || height_ <= 0) return false;

    // GDI+ 초기화 (lazy, 스레드 안전)
    static ULONG_PTR gdipToken = 0;
    static bool gdipInited = false;
    if (!gdipInited) {
        Gdiplus::GdiplusStartupInput input;
        Gdiplus::GdiplusStartup(&gdipToken, &input, nullptr);
        gdipInited = true;
    }

    CLSID clsid;
    if (!GetPngEncoderClsid(&clsid)) return false;

    // Bitmap 생성 (BGRA 픽셀 직접 참조)
    Gdiplus::Bitmap bmp(width_, height_, width_ * 4,
                        PixelFormat32bppRGB,
                        reinterpret_cast<BYTE*>(pixels_));

    return bmp.Save(path, &clsid, nullptr) == Gdiplus::Ok;
}

} // namespace wsse
