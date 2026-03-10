#include "framework/Registry.h"
#include "framework/FrameworkSettings.h"

namespace wsse {

const wchar_t* Registry::keyPath_ = L"Software\\ScreenSaver";

void Registry::SetKeyPath(const wchar_t* path) {
    keyPath_ = path;
}

// ---- 범용 읽기/쓰기 ----

bool Registry::ReadBool(const wchar_t* name, bool defaultVal) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, keyPath_, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return defaultVal;

    DWORD val = 0;
    DWORD size = sizeof(val);
    bool result = defaultVal;
    if (RegQueryValueExW(hKey, name, nullptr, nullptr,
                         reinterpret_cast<BYTE*>(&val), &size) == ERROR_SUCCESS) {
        result = val != 0;
    }
    RegCloseKey(hKey);
    return result;
}

int Registry::ReadInt(const wchar_t* name, int defaultVal) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, keyPath_, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return defaultVal;

    DWORD val = 0;
    DWORD size = sizeof(val);
    int result = defaultVal;
    if (RegQueryValueExW(hKey, name, nullptr, nullptr,
                         reinterpret_cast<BYTE*>(&val), &size) == ERROR_SUCCESS) {
        result = static_cast<int>(val);
    }
    RegCloseKey(hKey);
    return result;
}

void Registry::WriteBool(const wchar_t* name, bool val) {
    HKEY hKey;
    DWORD disp;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, keyPath_, 0, nullptr,
                        REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, &disp) != ERROR_SUCCESS)
        return;

    DWORD dw = val ? 1 : 0;
    RegSetValueExW(hKey, name, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&dw), sizeof(dw));
    RegCloseKey(hKey);
}

void Registry::WriteInt(const wchar_t* name, int val) {
    HKEY hKey;
    DWORD disp;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, keyPath_, 0, nullptr,
                        REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, &disp) != ERROR_SUCCESS)
        return;

    DWORD dw = static_cast<DWORD>(val);
    RegSetValueExW(hKey, name, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&dw), sizeof(dw));
    RegCloseKey(hKey);
}

// ---- 프레임워크 설정 ----

void Registry::LoadFrameworkSettings(FrameworkSettings& settings) {
    settings.forceCPU = ReadBool(L"ForceCPU", false);
    settings.showContent = ReadBool(L"ShowContent", true);
    settings.showOverlay = ReadBool(L"ShowOverlay", true);
    settings.showClock = ReadBool(L"ShowClock", true);
}

void Registry::SaveFrameworkSettings(const FrameworkSettings& settings) {
    WriteBool(L"ForceCPU", settings.forceCPU);
    WriteBool(L"ShowContent", settings.showContent);
    WriteBool(L"ShowOverlay", settings.showOverlay);
    WriteBool(L"ShowClock", settings.showClock);
}

} // namespace wsse
