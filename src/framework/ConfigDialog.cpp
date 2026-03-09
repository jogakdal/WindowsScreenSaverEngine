#include "framework/ConfigDialog.h"
#include "framework/FrameworkSettings.h"
#include "framework/AppDescriptor.h"
#include "framework/IScreenSaverContent.h"
#include "framework/Registry.h"
#include "framework/UpdateChecker.h"
#include "framework/Localization.h"
#include "framework/ControlIds.h"
#include <commctrl.h>
#include <shellapi.h>
#include <cstdio>
#include <new>

#pragma comment(lib, "version.lib")

namespace wsse {

FrameworkSettings* ConfigDialog::s_settings = nullptr;
IScreenSaverContent* ConfigDialog::s_content = nullptr;
const AppDescriptor* ConfigDialog::s_desc = nullptr;
static wchar_t s_updateUrl[512] = {};
static bool s_updateAvailable = false;

bool ConfigDialog::Show(HINSTANCE hInst, HWND parent,
                        const AppDescriptor& desc,
                        FrameworkSettings& settings,
                        IScreenSaverContent* content) {
    s_settings = &settings;
    s_content = content;
    s_desc = &desc;
    s_updateUrl[0] = L'\0';
    s_updateAvailable = false;
    INT_PTR result = DialogBoxParamW(hInst, MAKEINTRESOURCEW(desc.dialogResourceId),
                                     parent, DlgProc, 0);
    s_settings = nullptr;
    s_content = nullptr;
    s_desc = nullptr;
    return result == IDOK;
}

INT_PTR CALLBACK ConfigDialog::DlgProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    (void)lp;
    switch (msg) {
    case WM_INITDIALOG: {
        // 다이얼로그 제목: "%s Settings" (로컬라이즈)
        wchar_t title[256];
        swprintf_s(title, TR(Str::DlgTitleFmt), s_desc->appTitle);
        SetWindowTextW(hwnd, title);

        // 프레임워크 공통 컨트롤 텍스트 설정
        SetDlgItemTextW(hwnd, IDC_GRP_RENDERING, TR(Str::GrpRendering));
        SetDlgItemTextW(hwnd, IDC_FORCE_CPU, TR(Str::ChkForceCPU));
        SetDlgItemTextW(hwnd, IDC_GRP_DISPLAY, TR(Str::GrpDisplay));
        SetDlgItemTextW(hwnd, IDC_SHOW_OVERLAY, TR(Str::ChkOverlay));
        SetDlgItemTextW(hwnd, IDC_SHOW_CLOCK, TR(Str::ChkClock));
        SetDlgItemTextW(hwnd, IDC_GRP_UPDATE, TR(Str::GrpUpdate));
        SetDlgItemTextW(hwnd, IDC_UPDATE_CHECK, TR(Str::BtnCheckUpdate));
        SetDlgItemTextW(hwnd, IDOK, TR(Str::BtnOK));
        SetDlgItemTextW(hwnd, IDCANCEL, TR(Str::BtnCancel));

        // 버전 정보 표시
        {
            wchar_t exePath[MAX_PATH];
            GetModuleFileNameW(nullptr, exePath, MAX_PATH);
            DWORD dummy = 0;
            DWORD verSize = GetFileVersionInfoSizeW(exePath, &dummy);
            if (verSize > 0) {
                BYTE* buf = new (std::nothrow) BYTE[verSize];
                if (buf && GetFileVersionInfoW(exePath, 0, verSize, buf)) {
                    VS_FIXEDFILEINFO* ffi = nullptr;
                    UINT ffiLen = 0;
                    if (VerQueryValueW(buf, L"\\", reinterpret_cast<void**>(&ffi), &ffiLen)) {
                        wchar_t verText[128];
                        swprintf_s(verText, L"%s v%u.%u.%u",
                            s_desc->appTitle,
                            HIWORD(ffi->dwFileVersionMS),
                            LOWORD(ffi->dwFileVersionMS),
                            HIWORD(ffi->dwFileVersionLS));
                        SetDlgItemTextW(hwnd, IDC_VERSION_LABEL, verText);
                    }
                }
                delete[] buf;
            }
        }

        // 프레임워크 설정 컨트롤 초기화
        CheckDlgButton(hwnd, IDC_FORCE_CPU, s_settings->forceCPU ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd, IDC_SHOW_OVERLAY, s_settings->showOverlay ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd, IDC_SHOW_CLOCK, s_settings->showClock ? BST_CHECKED : BST_UNCHECKED);

        // 기능 플래그에 따라 불필요한 컨트롤 숨김
        auto HideControl = [&](int id) {
            HWND ctrl = GetDlgItem(hwnd, id);
            if (ctrl) ShowWindow(ctrl, SW_HIDE);
        };

        if (!s_desc->hasOverlay)
            HideControl(IDC_SHOW_OVERLAY);
        if (!s_desc->hasClock)
            HideControl(IDC_SHOW_CLOCK);
        if (!s_desc->hasOverlay && !s_desc->hasClock)
            HideControl(IDC_GRP_DISPLAY);

        if (!s_desc->hasAutoUpdate) {
            HideControl(IDC_GRP_UPDATE);
            HideControl(IDC_UPDATE_STATUS);
            HideControl(IDC_UPDATE_CHECK);
        }

        // 콘텐츠 고유 컨트롤 초기화
        if (s_content) s_content->InitConfigControls(hwnd);

        // 업데이트 자동 확인
        if (s_desc->hasAutoUpdate) {
            SetDlgItemTextW(hwnd, IDC_UPDATE_STATUS, TR(Str::Checking));
            CheckForUpdateAsync(hwnd, s_desc->updateHost, s_desc->updatePath, s_desc->userAgent);
        }

        return TRUE;
    }

    case WM_HSCROLL:
        // 콘텐츠에 메시지 위임
        if (s_content && s_content->HandleConfigMessage(hwnd, msg, wp, lp))
            return TRUE;
        return FALSE;

    case WM_UPDATE_RESULT: {
        if (wp == UPDATE_AVAILABLE) {
            UpdateInfo* info = reinterpret_cast<UpdateInfo*>(lp);
            if (info) {
                wchar_t statusMsg[128];
                swprintf_s(statusMsg, TR(Str::NewVersion), info->version);
                SetDlgItemTextW(hwnd, IDC_UPDATE_STATUS, statusMsg);
                wcscpy_s(s_updateUrl, info->url);
                s_updateAvailable = true;
                SetDlgItemTextW(hwnd, IDC_UPDATE_CHECK, TR(Str::BtnDownload));
                delete info;
            }
        } else if (wp == UPDATE_NONE) {
            SetDlgItemTextW(hwnd, IDC_UPDATE_STATUS, TR(Str::UpToDate));
        } else {
            SetDlgItemTextW(hwnd, IDC_UPDATE_STATUS, TR(Str::CheckFailed));
        }
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDC_UPDATE_CHECK:
            if (s_updateAvailable && s_updateUrl[0]) {
                ShellExecuteW(nullptr, L"open", s_updateUrl, nullptr, nullptr, SW_SHOWNORMAL);
                HWND hParent = GetParent(hwnd);
                EndDialog(hwnd, IDCANCEL);
                if (hParent)
                    PostMessageW(hParent, WM_CLOSE, 0, 0);
            } else {
                SetDlgItemTextW(hwnd, IDC_UPDATE_STATUS, TR(Str::Checking));
                CheckForUpdateAsync(hwnd, s_desc->updateHost, s_desc->updatePath, s_desc->userAgent);
            }
            return TRUE;

        case IDOK: {
            // 프레임워크 설정 읽기
            s_settings->forceCPU = IsDlgButtonChecked(hwnd, IDC_FORCE_CPU) == BST_CHECKED;
            if (s_desc->hasOverlay)
                s_settings->showOverlay = IsDlgButtonChecked(hwnd, IDC_SHOW_OVERLAY) == BST_CHECKED;
            if (s_desc->hasClock)
                s_settings->showClock = IsDlgButtonChecked(hwnd, IDC_SHOW_CLOCK) == BST_CHECKED;
            Registry::SaveFrameworkSettings(*s_settings);

            // 콘텐츠 고유 컨트롤 읽기 및 저장
            if (s_content) {
                s_content->ReadConfigControls(hwnd);
                s_content->SaveSettings();
            }

            EndDialog(hwnd, IDOK);
            return TRUE;
        }
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            return TRUE;
        }
        break;

    case WM_CLOSE:
        EndDialog(hwnd, IDCANCEL);
        return TRUE;
    }
    return FALSE;
}

} // namespace wsse
