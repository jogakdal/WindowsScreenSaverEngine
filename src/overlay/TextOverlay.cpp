#include "overlay/TextOverlay.h"
#include "overlay/SystemInfo.h"
#include "framework/Localization.h"
#include <cmath>
#include <cstdio>
#include <algorithm>

namespace wsse {

// ---- 오버레이 표시 상수 ----

// 폰트
constexpr int    kFontMin = 12;              // 최소 폰트 크기 (px)
constexpr int    kFontDivisor = 75;          // 폰트 크기 = renderH / 이 값
constexpr int    kMarginLeft = 10;           // 좌측 여백
constexpr int    kMarginTop = 8;             // 상단 여백
constexpr int    kMarginRight = 20;          // 바운스 우측 여백
constexpr int    kLineSpacing = 3;           // 줄 간격 추가분
constexpr int    kOutlineThickness = 2;      // 텍스트 외곽선 두께 (px)
constexpr int    kAlpha = 38;               // 불투명도 ~15% (시계와 동일)
constexpr int    kMaxCharsEstimate = 80;     // 동적 라인 최대 문자 수 추정치
constexpr double kBounceSpeed = 0.01;        // 코사인파 속도 (~10분 주기)

// 외곽선 텍스트 렌더링 (검은 외곽선 + 컬러 채움)
static void DrawOutlinedText(HDC hdc, int x, int y, const wchar_t* text, int len,
                             COLORREF fillColor) {
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(0, 0, 0));
    for (int dy = -kOutlineThickness; dy <= kOutlineThickness; dy++)
        for (int dx = -kOutlineThickness; dx <= kOutlineThickness; dx++)
            if (dx != 0 || dy != 0)
                TextOutW(hdc, x + dx, y + dy, text, len);
    SetTextColor(hdc, fillColor);
    TextOutW(hdc, x, y, text, len);
}

TextOverlay::~TextOverlay() {
    if (font_) DeleteObject(font_);
}

void TextOverlay::Init(int renderW, int renderH, int screenW, int screenH,
                       HDC surfDC, const SystemInfo& sysInfo, bool isLite) {
    QueryPerformanceFrequency(&perfFreq_);
    QueryPerformanceCounter(&bounceStart_);

    // 폰트 생성 (Consolas 고정폭)
    fontSize_ = (std::max)(kFontMin, renderH / kFontDivisor);
    font_ = CreateFontW(
        -fontSize_, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");

    // 시스템 정보 문자열 저장
    swprintf_s(gpuName_, L"GPU: %s", sysInfo.GetGPUName());
    swprintf_s(cpuName_, L"CPU: %s", sysInfo.GetCPUName());

    if (isLite) {
        swprintf_s(sysLine3_, L"RAM: %d GB | %d\u00D7%d (\u2192%d\u00D7%d Lite)",
                   sysInfo.GetRAMGB(), screenW, screenH, renderW, renderH);
    } else {
        swprintf_s(sysLine3_, L"RAM: %d GB | %d\u00D7%d",
                   sysInfo.GetRAMGB(), renderW, renderH);
    }

    // 수평 바운스 가동 범위 계산 (고정폭 폰트 기준)
    HFONT oldFont = static_cast<HFONT>(SelectObject(surfDC, font_));
    SIZE charSz;
    GetTextExtentPoint32W(surfDC, L"M", 1, &charSz);
    SelectObject(surfDC, oldFont);

    int charW = charSz.cx;
    int maxChars = (std::max)({
        static_cast<int>(wcslen(gpuName_)),
        static_cast<int>(wcslen(cpuName_)),
        static_cast<int>(wcslen(sysLine3_)),
        kMaxCharsEstimate
    });
    overlayRange_ = (std::max)(0, renderW - maxChars * charW - kMarginRight);
}

void TextOverlay::Render(HDC surfDC, float fadeAlpha, const wchar_t* dynamicLine) {
    // 수평 바운스: 코사인파, 좌측부터 시작, ~10분 주기
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    double elapsed = static_cast<double>(now.QuadPart - bounceStart_.QuadPart) / perfFreq_.QuadPart;
    double t = (1.0 - std::cos(elapsed * kBounceSpeed)) * 0.5;
    int ox = kMarginLeft + static_cast<int>(t * overlayRange_);

    // 페이드 알파에 따른 텍스트 색상 (시계와 동일한 밝기)
    int alpha = static_cast<int>(fadeAlpha * kAlpha);
    alpha = (std::min)(255, (std::max)(0, alpha));
    COLORREF textColor = RGB(alpha, alpha, alpha);

    HFONT oldFont = static_cast<HFONT>(SelectObject(surfDC, font_));
    int y = kMarginTop;
    int lineH = fontSize_ + kLineSpacing;

    // 고정 시스템 정보 (GPU / CPU / RAM+해상도)
    DrawOutlinedText(surfDC, ox, y, gpuName_,
                     static_cast<int>(wcslen(gpuName_)), textColor);
    y += lineH;
    DrawOutlinedText(surfDC, ox, y, cpuName_,
                     static_cast<int>(wcslen(cpuName_)), textColor);
    y += lineH;
    DrawOutlinedText(surfDC, ox, y, sysLine3_,
                     static_cast<int>(wcslen(sysLine3_)), textColor);
    y += lineH;

    // 콘텐츠가 제공하는 동적 정보 라인
    if (dynamicLine && dynamicLine[0]) {
        DrawOutlinedText(surfDC, ox, y, dynamicLine,
                         static_cast<int>(wcslen(dynamicLine)), textColor);
        y += lineH;
    }

    // F1 안내
    const wchar_t* helpLine = TR(Str::PressF1);
    DrawOutlinedText(surfDC, ox, y, helpLine,
                     static_cast<int>(wcslen(helpLine)), textColor);

    SelectObject(surfDC, oldFont);
}

void TextOverlay::RenderHelpLine(HDC surfDC, float fadeAlpha) {
    // 수평 바운스 위치 계산
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    double elapsed = static_cast<double>(now.QuadPart - bounceStart_.QuadPart) / perfFreq_.QuadPart;
    double t = (1.0 - std::cos(elapsed * kBounceSpeed)) * 0.5;
    int ox = kMarginLeft + static_cast<int>(t * overlayRange_);

    int alpha = static_cast<int>(fadeAlpha * kAlpha);
    alpha = (std::min)(255, (std::max)(0, alpha));
    COLORREF textColor = RGB(alpha, alpha, alpha);

    HFONT oldFont = static_cast<HFONT>(SelectObject(surfDC, font_));

    const wchar_t* helpLine = TR(Str::PressF1);
    DrawOutlinedText(surfDC, ox, kMarginTop, helpLine,
                     static_cast<int>(wcslen(helpLine)), textColor);

    SelectObject(surfDC, oldFont);
}

} // namespace wsse
