#pragma once
#include <windows.h>

namespace wsse {

// 7-세그먼트 디스플레이 설정
struct SegmentDisplayConfig {
    int digitW;          // 숫자 폭
    int digitH;          // 숫자 높이
    int segThick;        // 세그먼트 두께
    int segGap;          // 세그먼트 간격
    int digitSpacing;    // 숫자 간 간격
    int colonW;          // 콜론 폭
    int dimBrightness;   // 꺼진 세그먼트 밝기
};

// 7-세그먼트 시계 (HH:MM) 그리기 -- 비트맵 영역 중앙 정렬
void Draw7SegClock(HDC hdc, int bmpW, int bmpH,
                   const SegmentDisplayConfig& cfg,
                   int hour, int minute);

} // namespace wsse
