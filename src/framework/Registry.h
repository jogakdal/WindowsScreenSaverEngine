#pragma once
#include <windows.h>

namespace wsse {

struct FrameworkSettings;

// 레지스트리 설정 관리
//
// 앱 시작 시 SetKeyPath()로 키 경로를 설정한 후 사용.
// 프레임워크 설정은 LoadFrameworkSettings/SaveFrameworkSettings로,
// 콘텐츠 고유 설정은 ReadBool/ReadInt/WriteBool/WriteInt로 관리합니다.
class Registry {
public:
    // 레지스트리 키 경로 설정 (HKCU 하위, 앱 시작 시 한 번 호출)
    // e.g., "Software\\MyScreenSaver"
    static void SetKeyPath(const wchar_t* path);
    static const wchar_t* GetKeyPath() { return keyPath_; }

    // ---- 프레임워크 설정 ----

    static void LoadFrameworkSettings(FrameworkSettings& settings);
    static void SaveFrameworkSettings(const FrameworkSettings& settings);

    // ---- 범용 레지스트리 접근 (콘텐츠용) ----

    static bool ReadBool(const wchar_t* name, bool defaultVal);
    static int ReadInt(const wchar_t* name, int defaultVal);
    static void WriteBool(const wchar_t* name, bool val);
    static void WriteInt(const wchar_t* name, int val);

private:
    static const wchar_t* keyPath_;
};

} // namespace wsse
