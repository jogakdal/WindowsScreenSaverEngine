#pragma once

namespace wsse {

// 프레임워크 공통 설정
//
// 모든 스크린세이버에서 공유하는 설정 항목.
// 콘텐츠 고유 설정은 IScreenSaverContent::LoadSettings/SaveSettings로 관리합니다.
struct FrameworkSettings {
    bool forceCPU = false;       // GPU 대신 CPU 렌더링 강제
    bool showContent = true;     // 콘텐츠 애니메이션 표시
    bool showOverlay = true;     // 시스템 정보 오버레이 표시
    bool showClock = true;       // 디지털 시계 표시

    bool operator==(const FrameworkSettings& o) const {
        return forceCPU == o.forceCPU && showContent == o.showContent
            && showOverlay == o.showOverlay && showClock == o.showClock;
    }
    bool operator!=(const FrameworkSettings& o) const { return !(*this == o); }
};

} // namespace wsse
