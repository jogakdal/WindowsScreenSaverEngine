#pragma once
#include <windows.h>

namespace wsse {

// PDH API 기반 프로세스별 GPU 사용율 측정
// Windows 10 1709+ GPU 성능 카운터 사용
// 현재 프로세스의 3D 엔진 사용율을 0~100%로 반환
class GPUUsage {
public:
    GPUUsage() = default;
    ~GPUUsage();

    // 초기화: PDH 쿼리 생성 + 카운터 등록
    bool Init();

    // 이전 상태 정리 후 재초기화 대기 (설정 변경 시)
    void Reset();

    // 주기적 호출 (1초 간격 권장): 카운터 샘플링 + 사용율 갱신
    void Update();

    // 최근 측정된 GPU 사용율 (0~100, 미지원 시 -1)
    int GetUsagePercent() const { return usagePercent_; }

private:
    // PDH 핸들 (pdh.h 의존성을 cpp에 격리하기 위해 void*로 저장)
    void* query_ = nullptr;
    void** counters_ = nullptr;
    int counterCount_ = 0;
    int usagePercent_ = 0;
    bool initialized_ = false;
};

} // namespace wsse
