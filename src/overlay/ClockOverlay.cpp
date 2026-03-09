#include "overlay/ClockOverlay.h"
#include "render/SegmentDisplay.h"
#include <cmath>
#include <algorithm>

namespace wsse {

// ---- 시계 오버레이 상수 ----
constexpr int    kDigitMin = 36;              // 최소 숫자 높이 (px)
constexpr int    kDigitDivisor = 3;           // 숫자 높이 = renderH / 이 값
constexpr int    kBmpPadding = 8;             // 비트맵 패딩 (px)
constexpr int    kAlpha = 38;                 // 불투명도 ~15% (0~255)
constexpr float  kAspectRatio = 0.6f;         // 숫자 가로/세로 비율
constexpr float  kThicknessRatio = 0.12f;     // 세그먼트 두께 비율
constexpr float  kGapRatio = 0.02f;           // 세그먼트 간격 비율
constexpr float  kSpacingRatio = 0.12f;       // 숫자 간 간격 비율
constexpr float  kColonRatio = 0.35f;         // 콜론 폭 비율
constexpr int    kDimBrightness = 40;         // 꺼진 세그먼트 밝기 (0~255)
constexpr double kBounceRate = 0.03;          // 바운스 속도 = renderH * 이 값 (px/s)

ClockOverlay::~ClockOverlay() {
    if (clockDC_) {
        SelectObject(clockDC_, clockOldBmp_);
        DeleteDC(clockDC_);
    }
    if (clockBmp_) DeleteObject(clockBmp_);
}

void ClockOverlay::Init(int renderW, int renderH) {
    QueryPerformanceFrequency(&perfFreq_);

    // 세그먼트 크기 계산
    digitH_ = (std::max)(kDigitMin, renderH / kDigitDivisor);
    digitW_ = static_cast<int>(digitH_ * kAspectRatio);
    segThick_ = (std::max)(2, static_cast<int>(digitH_ * kThicknessRatio));
    segGap_ = (std::max)(1, static_cast<int>(segThick_ * kGapRatio));
    digitSpacing_ = (std::max)(2, static_cast<int>(digitW_ * kSpacingRatio));
    colonW_ = (std::max)(2, static_cast<int>(digitW_ * kColonRatio));

    // 비트맵 크기: 숫자 4개 + 콜론 + 간격 + 패딩
    clockW_ = 4 * digitW_ + colonW_ + 4 * digitSpacing_ + kBmpPadding * 2;
    clockH_ = digitH_ + kBmpPadding * 2;

    // 32비트 DIB 섹션 생성 (per-pixel 알파용)
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = clockW_;
    bmi.bmiHeader.biHeight = -clockH_;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    clockBmp_ = CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS,
                                reinterpret_cast<void**>(&clockBits_), nullptr, 0);
    clockDC_ = CreateCompatibleDC(nullptr);
    clockOldBmp_ = SelectObject(clockDC_, clockBmp_);
    SelectObject(clockDC_, GetStockObject(WHITE_BRUSH));
    SelectObject(clockDC_, GetStockObject(WHITE_PEN));

    // 바운스 초기 위치 (화면 중앙) + 랜덤 방향
    posX_ = (renderW - clockW_) / 2.0;
    posY_ = (renderH - clockH_) / 2.0;
    QueryPerformanceCounter(&lastBounceTime_);
    double angle = (lastBounceTime_.QuadPart % 10000) / 10000.0 * 2.0 * 3.14159265358979;
    double speed = renderH * kBounceRate;
    velX_ = speed * std::cos(angle);
    velY_ = speed * std::sin(angle);
}

void ClockOverlay::Render(HDC surfDC, int renderW, int renderH) {
    if (!clockBits_) return;

    // 시각 확인 (분 단위 변경 시만 비트맵 재생성)
    SYSTEMTIME st;
    GetLocalTime(&st);
    int curTime = st.wHour * 60 + st.wMinute;

    if (curTime != lastClockTime_) {
        lastClockTime_ = curTime;
        memset(clockBits_, 0, clockW_ * clockH_ * 4);

        SegmentDisplayConfig segCfg = {
            digitW_, digitH_, segThick_, segGap_,
            digitSpacing_, colonW_, kDimBrightness
        };
        Draw7SegClock(clockDC_, clockW_, clockH_, segCfg, st.wHour, st.wMinute);

        // 사전 곱셈 알파 (premultiplied alpha) 적용
        int totalPixels = clockW_ * clockH_;
        for (int i = 0; i < totalPixels; i++) {
            BYTE r = clockBits_[i * 4 + 2];
            if (r > 0) {
                clockBits_[i * 4 + 0] = static_cast<BYTE>((clockBits_[i * 4 + 0] * kAlpha + 127) / 255);
                clockBits_[i * 4 + 1] = static_cast<BYTE>((clockBits_[i * 4 + 1] * kAlpha + 127) / 255);
                clockBits_[i * 4 + 2] = static_cast<BYTE>((r * kAlpha + 127) / 255);
                clockBits_[i * 4 + 3] = static_cast<BYTE>((r * kAlpha + 127) / 255);
            }
        }
    }

    // 바운스 위치 업데이트
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    double dt = static_cast<double>(now.QuadPart - lastBounceTime_.QuadPart) / perfFreq_.QuadPart;
    lastBounceTime_ = now;
    if (dt > 0.1) dt = 0.1;

    posX_ += velX_ * dt;
    posY_ += velY_ * dt;

    // 경계 반사
    int maxCX = renderW - clockW_;
    int maxCY = renderH - clockH_;
    if (maxCX > 0) {
        if (posX_ < 0) {
            posX_ = -posX_;
            velX_ = -velX_;
        }
        if (posX_ > maxCX) {
            posX_ = 2.0 * maxCX - posX_;
            velX_ = -velX_;
        }
    }
    if (maxCY > 0) {
        if (posY_ < 0) {
            posY_ = -posY_;
            velY_ = -velY_;
        }
        if (posY_ > maxCY) {
            posY_ = 2.0 * maxCY - posY_;
            velY_ = -velY_;
        }
    }

    // AlphaBlend로 반투명 합성
    BLENDFUNCTION blend = {};
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;
    AlphaBlend(surfDC, static_cast<int>(posX_), static_cast<int>(posY_),
               clockW_, clockH_, clockDC_, 0, 0, clockW_, clockH_, blend);
}

} // namespace wsse
