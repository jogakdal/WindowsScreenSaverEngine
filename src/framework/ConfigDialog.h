#pragma once
#include <windows.h>

namespace wsse {

struct FrameworkSettings;
struct AppDescriptor;
class IScreenSaverContent;

// 설정 다이얼로그
//
// 프레임워크 공통 컨트롤(CPU 강제, 오버레이, 시계, 업데이트)을 관리하고,
// 콘텐츠 고유 컨트롤은 IScreenSaverContent::InitConfigControls/ReadConfigControls에 위임합니다.
class ConfigDialog {
public:
    static bool Show(HINSTANCE hInst, HWND parent,
                     const AppDescriptor& desc,
                     FrameworkSettings& settings,
                     IScreenSaverContent* content = nullptr);

private:
    static INT_PTR CALLBACK DlgProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    static FrameworkSettings* s_settings;
    static IScreenSaverContent* s_content;
    static const AppDescriptor* s_desc;
};

} // namespace wsse
