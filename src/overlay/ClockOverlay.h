#pragma once
#include <windows.h>

namespace wsse {

// 7-세그먼트 디지털 시계 오버레이
// 반투명 AlphaBlend 합성, 바운스 물리로 번인 방지
class ClockOverlay {
public:
    ~ClockOverlay();

    // 비트맵 및 세그먼트 설정 초기화, 바운스 시작 위치/속도 결정
    void Init(int renderW, int renderH);

    // 시계 렌더링: 현재 시각 표시 + 바운스 위치 업데이트 + AlphaBlend 합성
    void Render(HDC surfDC, int renderW, int renderH);

private:
    HDC clockDC_ = nullptr;
    HBITMAP clockBmp_ = nullptr;
    HGDIOBJ clockOldBmp_ = nullptr;
    BYTE* clockBits_ = nullptr;
    int clockW_ = 0, clockH_ = 0;

    // 세그먼트 크기 설정
    int digitW_ = 0, digitH_ = 0;
    int segThick_ = 0, segGap_ = 0;
    int digitSpacing_ = 0, colonW_ = 0;

    // 시각 캐싱 (분 단위 변경 시만 비트맵 재생성)
    int lastClockTime_ = -1;

    // 바운스 물리
    double posX_ = 0.0, posY_ = 0.0;
    double velX_ = 0.0, velY_ = 0.0;
    LARGE_INTEGER lastBounceTime_{};
    LARGE_INTEGER perfFreq_{};
};

} // namespace wsse
