#include "overlay/TextOverlay.h"
#include "overlay/SystemInfo.h"
#include "framework/Localization.h"
#include <cmath>
#include <cstdio>
#include <algorithm>

namespace wsse {

// ---- 오버레이 표시 상수 ----

constexpr int    kFontMin = 12;              // 최소 폰트 크기 (px)
constexpr int    kFontDivisor = 75;          // 폰트 크기 = renderH / 이 값
constexpr int    kMarginLeft = 10;           // 좌측 여백
constexpr int    kMarginTop = 8;             // 상단 여백
constexpr int    kMarginRight = 20;          // 바운스 우측 여백
constexpr int    kLineSpacing = 3;           // 줄 간격 추가분
constexpr int    kMaxCharsEstimate = 80;     // 동적 라인 최대 문자 수 추정치
constexpr double kBounceSpeed = 0.01;        // 코사인파 속도 (~10분 주기)
constexpr int    kBmpPadding = 4;            // 비트맵 패딩 (px)
constexpr int    kTextAlpha = 50;            // 텍스트 불투명도 ~20%
constexpr int    kShadowAlpha = 60;          // 그림자 불투명도 ~24%
constexpr int    kShadowThick = 1;           // 그림자 외곽 두께 (px)

TextOverlay::~TextOverlay() {
    if (textDC_) {
        SelectObject(textDC_, textOldBmp_);
        DeleteDC(textDC_);
    }
    if (textBmp_) DeleteObject(textBmp_);
    if (font_) DeleteObject(font_);
}

void TextOverlay::Init(int renderW, int renderH, int screenW, int screenH,
                       HDC surfDC, const SystemInfo& sysInfo, bool isLite) {
    QueryPerformanceFrequency(&perfFreq_);
    QueryPerformanceCounter(&bounceStart_);

    // 폰트 생성 (Consolas 고정폭, DIB 렌더링용 그레이스케일 AA)
    fontSize_ = (std::max)(kFontMin, renderH / kFontDivisor);
    font_ = CreateFontW(
        -fontSize_, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");

    // 시스템 정보 문자열 저장: CPU/GPU 각각 1줄로 통합
    swprintf_s(cpuLine_, L"CPU: %s | RAM: %d GB",
               sysInfo.GetCPUName(), sysInfo.GetRAMGB());
    if (isLite) {
        swprintf_s(gpuLine_, L"GPU: %s | %d\u00D7%d (\u2192%d\u00D7%d Lite)",
                   sysInfo.GetGPUName(), screenW, screenH, renderW, renderH);
    } else {
        swprintf_s(gpuLine_, L"GPU: %s | %d\u00D7%d",
                   sysInfo.GetGPUName(), renderW, renderH);
    }

    // 수평 바운스 가동 범위 계산 (고정폭 폰트 기준)
    HFONT oldFont = static_cast<HFONT>(SelectObject(surfDC, font_));
    SIZE charSz;
    GetTextExtentPoint32W(surfDC, L"M", 1, &charSz);
    SelectObject(surfDC, oldFont);

    int charW = charSz.cx;
    int maxChars = (std::max)({
        static_cast<int>(wcslen(cpuLine_)),
        static_cast<int>(wcslen(gpuLine_)),
        kMaxCharsEstimate
    });
    overlayRange_ = (std::max)(0, renderW - maxChars * charW - kMarginRight);

    // 반투명 합성용 32비트 DIB 비트맵 생성
    int lineH = fontSize_ + kLineSpacing;
    textBmpW_ = maxChars * charW + kBmpPadding * 2;
    textBmpH_ = 6 * lineH + kBmpPadding * 2;

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = textBmpW_;
    bmi.bmiHeader.biHeight = -textBmpH_;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    textBmp_ = CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS,
                                reinterpret_cast<void**>(&textBits_), nullptr, 0);
    textDC_ = CreateCompatibleDC(nullptr);
    textOldBmp_ = SelectObject(textDC_, textBmp_);
}

void TextOverlay::BlendToSurface(HDC surfDC, int dstX, int dstY, int h) {
    BLENDFUNCTION blend = {};
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;
    AlphaBlend(surfDC, dstX, dstY, textBmpW_, h,
               textDC_, 0, 0, textBmpW_, h, blend);
}

void TextOverlay::Render(HDC surfDC, float fadeAlpha, const wchar_t* dynamicLine,
                         double cpuUsage, int gpuLoad) {
    (void)fadeAlpha;
    if (!textBits_) return;

    // CPU 라인: cpuLine_ 기반, " | RAM:" 앞에 suffix 삽입
    wchar_t cpuLine[320];
    wchar_t cpuSuffix[64];
    swprintf_s(cpuSuffix, TR(Str::CpuInUse), cpuUsage);
    const wchar_t* ramPart = wcsstr(cpuLine_, L" | ");
    if (ramPart) {
        int nameLen = static_cast<int>(ramPart - cpuLine_);
        _snwprintf_s(cpuLine, _TRUNCATE, L"%.*s%s%s", nameLen, cpuLine_, cpuSuffix, ramPart);
    } else {
        _snwprintf_s(cpuLine, _TRUNCATE, L"%s%s", cpuLine_, cpuSuffix);
    }

    // GPU 라인: gpuLine_ 기반, " | " 앞에 GPU 부하 삽입
    wchar_t gpuLine[320];
    if (gpuLoad >= 0) {
        wchar_t gpuSuffix[64];
        swprintf_s(gpuSuffix, TR(Str::GpuInUse), gpuLoad);
        const wchar_t* resPart = wcsstr(gpuLine_, L" | ");
        if (resPart) {
            int nameLen = static_cast<int>(resPart - gpuLine_);
            _snwprintf_s(gpuLine, _TRUNCATE, L"%.*s%s%s", nameLen, gpuLine_, gpuSuffix, resPart);
        } else {
            _snwprintf_s(gpuLine, _TRUNCATE, L"%s%s", gpuLine_, gpuSuffix);
        }
    } else {
        wcscpy_s(gpuLine, gpuLine_);
    }

    // 수평 바운스
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    double elapsed = static_cast<double>(now.QuadPart - bounceStart_.QuadPart) / perfFreq_.QuadPart;
    double t = (1.0 - std::cos(elapsed * kBounceSpeed)) * 0.5;
    int ox = kMarginLeft + static_cast<int>(t * overlayRange_);

    HFONT oldFont = static_cast<HFONT>(SelectObject(textDC_, font_));
    SetBkMode(textDC_, TRANSPARENT);
    SetTextColor(textDC_, RGB(255, 255, 255));

    int lineH = fontSize_ + kLineSpacing;
    const wchar_t* helpLine = TR(Str::PressF1);

    // 텍스트 라인 렌더링 헬퍼
    auto renderLines = [&](int dx, int dy) {
        int x = kBmpPadding + dx;
        int y = kBmpPadding + dy;
        TextOutW(textDC_, x, y, cpuLine, static_cast<int>(wcslen(cpuLine)));
        y += lineH;
        TextOutW(textDC_, x, y, gpuLine, static_cast<int>(wcslen(gpuLine)));
        y += lineH;
        if (dynamicLine && dynamicLine[0]) {
            TextOutW(textDC_, x, y, dynamicLine, static_cast<int>(wcslen(dynamicLine)));
            y += lineH;
        }
        TextOutW(textDC_, x, y, helpLine, static_cast<int>(wcslen(helpLine)));
    };

    // 사용 영역 높이
    int numLines = 3; // CPU, GPU, F1
    if (dynamicLine && dynamicLine[0]) numLines++;
    int usedH = (std::min)(kBmpPadding * 2 + numLines * lineH, textBmpH_);
    int totalPixels = textBmpW_ * usedH;
    int dstX = ox - kBmpPadding;
    int dstY = kMarginTop - kBmpPadding;

    // Pass 1: 그림자 (밝은 배경에서 대비 제공)
    // 흰색 텍스트를 외곽 위치에 렌더링 후, 검은 그림자로 변환
    memset(textBits_, 0, textBmpW_ * usedH * 4);
    for (int sdy = -kShadowThick; sdy <= kShadowThick; sdy++) {
        for (int sdx = -kShadowThick; sdx <= kShadowThick; sdx++) {
            if (sdx == 0 && sdy == 0) continue;
            renderLines(sdx, sdy);
        }
    }
    // premultiplied alpha: RGB=0 (검정), alpha만 유지 -> 배경을 어둡게
    for (int i = 0; i < totalPixels; i++) {
        BYTE r = textBits_[i * 4 + 2];
        if (r > 0) {
            BYTE a = static_cast<BYTE>((r * kShadowAlpha + 127) / 255);
            textBits_[i * 4 + 0] = 0;
            textBits_[i * 4 + 1] = 0;
            textBits_[i * 4 + 2] = 0;
            textBits_[i * 4 + 3] = a;
        }
    }
    BlendToSurface(surfDC, dstX, dstY, usedH);

    // Pass 2: 텍스트 (어두운 배경에서 대비 제공)
    memset(textBits_, 0, textBmpW_ * usedH * 4);
    renderLines(0, 0);
    for (int i = 0; i < totalPixels; i++) {
        BYTE r = textBits_[i * 4 + 2];
        if (r > 0) {
            textBits_[i * 4 + 0] = static_cast<BYTE>((textBits_[i * 4 + 0] * kTextAlpha + 127) / 255);
            textBits_[i * 4 + 1] = static_cast<BYTE>((textBits_[i * 4 + 1] * kTextAlpha + 127) / 255);
            textBits_[i * 4 + 2] = static_cast<BYTE>((r * kTextAlpha + 127) / 255);
            textBits_[i * 4 + 3] = static_cast<BYTE>((r * kTextAlpha + 127) / 255);
        }
    }
    BlendToSurface(surfDC, dstX, dstY, usedH);

    SelectObject(textDC_, oldFont);
}

void TextOverlay::RenderHelpLine(HDC surfDC, float fadeAlpha) {
    (void)fadeAlpha;
    if (!textBits_) return;

    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    double elapsed = static_cast<double>(now.QuadPart - bounceStart_.QuadPart) / perfFreq_.QuadPart;
    double t = (1.0 - std::cos(elapsed * kBounceSpeed)) * 0.5;
    int ox = kMarginLeft + static_cast<int>(t * overlayRange_);

    HFONT oldFont = static_cast<HFONT>(SelectObject(textDC_, font_));
    SetBkMode(textDC_, TRANSPARENT);
    SetTextColor(textDC_, RGB(255, 255, 255));

    const wchar_t* helpLine = TR(Str::PressF1);
    int helpLen = static_cast<int>(wcslen(helpLine));
    int lineH = fontSize_ + kLineSpacing;
    int usedH = lineH + kBmpPadding * 2;
    int totalPixels = textBmpW_ * usedH;
    int dstX = ox - kBmpPadding;
    int dstY = kMarginTop - kBmpPadding;

    // Pass 1: 그림자
    memset(textBits_, 0, textBmpW_ * usedH * 4);
    for (int sdy = -kShadowThick; sdy <= kShadowThick; sdy++) {
        for (int sdx = -kShadowThick; sdx <= kShadowThick; sdx++) {
            if (sdx == 0 && sdy == 0) continue;
            TextOutW(textDC_, kBmpPadding + sdx, kBmpPadding + sdy, helpLine, helpLen);
        }
    }
    for (int i = 0; i < totalPixels; i++) {
        BYTE r = textBits_[i * 4 + 2];
        if (r > 0) {
            BYTE a = static_cast<BYTE>((r * kShadowAlpha + 127) / 255);
            textBits_[i * 4 + 0] = 0;
            textBits_[i * 4 + 1] = 0;
            textBits_[i * 4 + 2] = 0;
            textBits_[i * 4 + 3] = a;
        }
    }
    BlendToSurface(surfDC, dstX, dstY, usedH);

    // Pass 2: 텍스트
    memset(textBits_, 0, textBmpW_ * usedH * 4);
    TextOutW(textDC_, kBmpPadding, kBmpPadding, helpLine, helpLen);
    for (int i = 0; i < totalPixels; i++) {
        BYTE r = textBits_[i * 4 + 2];
        if (r > 0) {
            textBits_[i * 4 + 0] = static_cast<BYTE>((textBits_[i * 4 + 0] * kTextAlpha + 127) / 255);
            textBits_[i * 4 + 1] = static_cast<BYTE>((textBits_[i * 4 + 1] * kTextAlpha + 127) / 255);
            textBits_[i * 4 + 2] = static_cast<BYTE>((r * kTextAlpha + 127) / 255);
            textBits_[i * 4 + 3] = static_cast<BYTE>((r * kTextAlpha + 127) / 255);
        }
    }
    BlendToSurface(surfDC, dstX, dstY, usedH);

    SelectObject(textDC_, oldFont);
}

} // namespace wsse
