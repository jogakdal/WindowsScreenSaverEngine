#pragma once
#include <windows.h>

namespace wsse {

class SystemInfo;

// 화면 좌상단 텍스트 오버레이
// 시스템 정보(GPU/CPU/RAM) + 콘텐츠가 제공하는 동적 정보
// 느린 코사인 바운스로 좌우 이동 (번인 방지)
// AlphaBlend 반투명 합성 (시계와 동일 방식)
class TextOverlay {
public:
    ~TextOverlay();

    // 초기화: 폰트 생성, 시스템 정보 라인 포맷, 바운스 범위 계산
    void Init(int renderW, int renderH, int screenW, int screenH,
              HDC surfDC, const SystemInfo& sysInfo, bool isLite);

    // 매 프레임 오버레이 렌더링
    // fadeAlpha: 미사용 (호환성 유지)
    // dynamicLine: 콘텐츠가 제공하는 동적 정보 문자열 (1줄)
    void Render(HDC surfDC, float fadeAlpha, const wchar_t* dynamicLine);

    // F1 안내 문구만 렌더링 (오버레이 꺼져 있어도 표시)
    void RenderHelpLine(HDC surfDC, float fadeAlpha);

private:
    HFONT font_ = nullptr;
    int fontSize_ = 0;
    int overlayRange_ = 0;         // 수평 바운스 가동 범위 (px)
    LARGE_INTEGER bounceStart_{};  // 바운스 시작 시각
    LARGE_INTEGER perfFreq_{};     // 성능 카운터 주파수

    // 반투명 합성용 비트맵
    HDC textDC_ = nullptr;
    HBITMAP textBmp_ = nullptr;
    HGDIOBJ textOldBmp_ = nullptr;
    BYTE* textBits_ = nullptr;
    int textBmpW_ = 0;
    int textBmpH_ = 0;

    // 고정 시스템 정보 문자열 (한 번만 생성)
    wchar_t gpuName_[256]{};
    wchar_t cpuName_[256]{};
    wchar_t sysLine3_[128]{};

    // 비트맵에 premultiplied alpha 적용 후 AlphaBlend
    void BlendToSurface(HDC surfDC, int dstX, int dstY, int h);
};

} // namespace wsse
