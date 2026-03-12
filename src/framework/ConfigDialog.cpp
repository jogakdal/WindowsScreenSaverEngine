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
HFONT ConfigDialog::s_titleFont = nullptr;
HFONT ConfigDialog::s_boldFont = nullptr;
int ConfigDialog::s_headerHeightPx = 0;
int ConfigDialog::s_footerTopPx = 0;
int ConfigDialog::s_footerBottomPx = 0;
static wchar_t s_updateUrl[512] = {};
static bool s_updateAvailable = false;
static const wchar_t* kSponsorLinks =
    L"<a href=\"https://buymeacoffee.com/jogakdal\">Buy Me a Coffee</a>  "
    L"<a href=\"https://github.com/sponsors/jogakdal\">GitHub Sponsors</a>";

// SysLink 컨트롤을 다이얼로그 내 수평 중앙으로 정렬
static void CenterSysLink(HWND dlg, int ctrlId) {
    HWND ctrl = GetDlgItem(dlg, ctrlId);
    if (!ctrl) return;

    // 표시 텍스트 추출 (HTML 태그 제거)
    wchar_t raw[512];
    GetWindowTextW(ctrl, raw, _countof(raw));
    wchar_t plain[512];
    int j = 0;
    bool inTag = false;
    for (const wchar_t* p = raw; *p; p++) {
        if (*p == L'<') { inTag = true; continue; }
        if (*p == L'>') { inTag = false; continue; }
        if (!inTag) plain[j++] = *p;
    }
    plain[j] = 0;

    // 텍스트 폭 측정
    HDC hdc = GetDC(ctrl);
    HFONT hFont = reinterpret_cast<HFONT>(SendMessageW(ctrl, WM_GETFONT, 0, 0));
    if (!hFont) hFont = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    HFONT old = static_cast<HFONT>(SelectObject(hdc, hFont));
    SIZE sz = {};
    GetTextExtentPoint32W(hdc, plain, j, &sz);
    SelectObject(hdc, old);
    ReleaseDC(ctrl, hdc);

    // 중앙 정렬
    RECT dlgRc;
    GetClientRect(dlg, &dlgRc);
    RECT rc;
    GetWindowRect(ctrl, &rc);
    MapWindowPoints(nullptr, dlg, reinterpret_cast<POINT*>(&rc), 2);
    int newX = (dlgRc.right - sz.cx) / 2;
    if (newX < 0) newX = rc.left;
    MoveWindow(ctrl, newX, rc.top, sz.cx + 8, rc.bottom - rc.top, FALSE);
}

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
    if (s_titleFont) { DeleteObject(s_titleFont); s_titleFont = nullptr; }
    if (s_boldFont) { DeleteObject(s_boldFont); s_boldFont = nullptr; }
    s_headerHeightPx = 0;
    s_footerTopPx = 0;
    s_footerBottomPx = 0;
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
        SetDlgItemTextW(hwnd, IDC_SHOW_CONTENT, TR(Str::ChkContent));
        SetDlgItemTextW(hwnd, IDC_SHOW_OVERLAY, TR(Str::ChkOverlay));
        SetDlgItemTextW(hwnd, IDC_SHOW_CLOCK, TR(Str::ChkClock));
        SetDlgItemTextW(hwnd, IDC_GRP_UPDATE, TR(Str::GrpUpdate));
        SetDlgItemTextW(hwnd, IDC_UPDATE_CHECK, TR(Str::BtnCheckUpdate));
        SetDlgItemTextW(hwnd, IDOK, TR(Str::BtnOK));
        SetDlgItemTextW(hwnd, IDCANCEL, TR(Str::BtnCancel));

        // 헤더 제목에 버전 포함: "AppTitle vX.Y.Z"
        {
            wchar_t verSuffix[64] = {};
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
                        swprintf_s(verSuffix, L" v%u.%u.%u",
                            HIWORD(ffi->dwFileVersionMS),
                            LOWORD(ffi->dwFileVersionMS),
                            HIWORD(ffi->dwFileVersionLS));
                    }
                }
                delete[] buf;
            }
            wchar_t headerText[256];
            swprintf_s(headerText, L"%s%s", s_desc->appTitle, verSuffix);
            SetDlgItemTextW(hwnd, IDC_HEADER_TITLE, headerText);
        }

        // 프레임워크 설정 컨트롤 초기화
        CheckDlgButton(hwnd, IDC_FORCE_CPU, s_settings->forceCPU ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd, IDC_SHOW_CONTENT, s_settings->showContent ? BST_CHECKED : BST_UNCHECKED);
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

        if (!s_desc->hasAutoUpdate) {
            HideControl(IDC_GRP_UPDATE);
            HideControl(IDC_UPDATE_STATUS);
            HideControl(IDC_UPDATE_CHECK);
        }

        // 콘텐츠 고유 컨트롤 초기화
        if (s_content) s_content->InitConfigControls(hwnd);

        // 프랙탈 렌더링 꺼짐 시 렌더링 섹션 비활성화
        if (!s_settings->showContent) {
            EnableWindow(GetDlgItem(hwnd, IDC_GRP_RENDERING), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_FORCE_CPU), FALSE);
        }

        // TaskDialog 스타일 헤더/폰트 설정
        {
            HFONT hDlgFont = reinterpret_cast<HFONT>(SendMessageW(hwnd, WM_GETFONT, 0, 0));
            LOGFONTW lf = {};
            GetObjectW(hDlgFont, sizeof(lf), &lf);

            // 헤더 제목 폰트: 50% 크게
            LOGFONTW titleLf = lf;
            titleLf.lfHeight = lf.lfHeight * 3 / 2;
            s_titleFont = CreateFontIndirectW(&titleLf);

            // 섹션 라벨 볼드 폰트
            LOGFONTW boldLf = lf;
            boldLf.lfWeight = FW_BOLD;
            s_boldFont = CreateFontIndirectW(&boldLf);

            // 폰트 적용 (프레임워크 컨트롤)
            SendDlgItemMessageW(hwnd, IDC_HEADER_TITLE, WM_SETFONT, reinterpret_cast<WPARAM>(s_titleFont), FALSE);
            SendDlgItemMessageW(hwnd, IDC_GRP_RENDERING, WM_SETFONT, reinterpret_cast<WPARAM>(s_boldFont), FALSE);
            SendDlgItemMessageW(hwnd, IDC_GRP_DISPLAY, WM_SETFONT, reinterpret_cast<WPARAM>(s_boldFont), FALSE);
            SendDlgItemMessageW(hwnd, IDC_GRP_UPDATE, WM_SETFONT, reinterpret_cast<WPARAM>(s_boldFont), FALSE);

            // 제작자/후원 정보
            SetDlgItemTextW(hwnd, IDC_AUTHOR_INFO, TR(Str::AuthorInfo));
            SetDlgItemTextW(hwnd, IDC_SPONSOR_MSG, TR(Str::SponsorMsg));
            SetDlgItemTextW(hwnd, IDC_SPONSOR_LINKS, kSponsorLinks);

            // SysLink 중앙 정렬
            CenterSysLink(hwnd, IDC_AUTHOR_INFO);
            CenterSysLink(hwnd, IDC_SPONSOR_LINKS);

            // 헤더/푸터 영역 높이 계산 (DLU -> 픽셀)
            RECT r = { 0, 0, 0, 42 };
            MapDialogRect(hwnd, &r);
            s_headerHeightPx = r.bottom;

            RECT fr = { 0, 216, 0, 264 };
            MapDialogRect(hwnd, &fr);
            s_footerTopPx = fr.top;
            s_footerBottomPx = fr.bottom;
        }

        // 업데이트 자동 확인
        if (s_desc->hasAutoUpdate) {
            SetDlgItemTextW(hwnd, IDC_UPDATE_STATUS, TR(Str::Checking));
            CheckForUpdateAsync(hwnd, s_desc->updateHost, s_desc->updatePath, s_desc->userAgent);
        }

        return TRUE;
    }

    case WM_ERASEBKGND: {
        HDC hdc = reinterpret_cast<HDC>(wp);
        RECT rc;
        GetClientRect(hwnd, &rc);
        FillRect(hdc, &rc, GetSysColorBrush(COLOR_3DFACE));
        if (s_headerHeightPx > 0) {
            RECT hdr = { 0, 0, rc.right, s_headerHeightPx };
            FillRect(hdc, &hdr, static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
        }
        if (s_footerTopPx > 0) {
            RECT ftr = { 0, s_footerTopPx, rc.right, s_footerBottomPx };
            FillRect(hdc, &ftr, static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
        }
        SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, 1);
        return TRUE;
    }

    case WM_CTLCOLORSTATIC: {
        int id = GetDlgCtrlID(reinterpret_cast<HWND>(lp));
        if (id == IDC_HEADER_TITLE) {
            HDC hdc = reinterpret_cast<HDC>(wp);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, GetSysColor(COLOR_HOTLIGHT));
            return reinterpret_cast<INT_PTR>(GetStockObject(WHITE_BRUSH));
        }
        if (id == IDC_AUTHOR_INFO || id == IDC_SPONSOR_MSG || id == IDC_SPONSOR_LINKS) {
            HDC hdc = reinterpret_cast<HDC>(wp);
            SetBkMode(hdc, TRANSPARENT);
            return reinterpret_cast<INT_PTR>(GetStockObject(WHITE_BRUSH));
        }
        return FALSE;
    }

    case WM_NOTIFY: {
        NMHDR* nmh = reinterpret_cast<NMHDR*>(lp);
        if (nmh->code == NM_CLICK || nmh->code == NM_RETURN) {
            if (nmh->idFrom == IDC_AUTHOR_INFO || nmh->idFrom == IDC_SPONSOR_LINKS) {
                NMLINK* link = reinterpret_cast<NMLINK*>(lp);
                if (link->item.szUrl[0]) {
                    ShellExecuteW(nullptr, L"open", link->item.szUrl,
                        nullptr, nullptr, SW_SHOWNORMAL);
                }
                return TRUE;
            }
        }
        return FALSE;
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
        case IDC_SHOW_CONTENT: {
            BOOL enabled = IsDlgButtonChecked(hwnd, IDC_SHOW_CONTENT) == BST_CHECKED;
            EnableWindow(GetDlgItem(hwnd, IDC_GRP_RENDERING), enabled);
            EnableWindow(GetDlgItem(hwnd, IDC_FORCE_CPU), enabled);
            if (s_content)
                s_content->HandleConfigMessage(hwnd, msg, wp, lp);
            return TRUE;
        }
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
            s_settings->showContent = IsDlgButtonChecked(hwnd, IDC_SHOW_CONTENT) == BST_CHECKED;
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
