#pragma once
#include <windows.h>

namespace wsse {

// 시스템 하드웨어 정보 수집기 (GPU, CPU, RAM)
// 앱 시작 시 한 번만 수집하여 오버레이에 표시
class SystemInfo {
public:
    void Collect();

    const wchar_t* GetGPUName() const { return gpuName_; }
    const wchar_t* GetCPUName() const { return cpuName_; }
    int GetRAMGB() const { return ramGB_; }

private:
    wchar_t gpuName_[256]{};
    wchar_t cpuName_[256]{};
    int ramGB_ = 0;
};

} // namespace wsse
