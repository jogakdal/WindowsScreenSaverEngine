#include "framework/ScreenSaverEngine.h"
#include "render/RenderSurface.h"
#include "framework/ScreenSaverWindow.h"
#include "framework/PreviewWindow.h"
#include "framework/ConfigDialog.h"
#include "framework/Registry.h"
#include "framework/Localization.h"
#include "framework/IPreviewContent.h"
#include "overlay/SystemInfo.h"
#include "overlay/TextOverlay.h"
#include "overlay/ClockOverlay.h"
#include <windowsx.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <commctrl.h>
#include <tlhelp32.h>
#include <cwchar>
#include <new>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "version.lib")

namespace wsse {

// ---- 프레임 타이밍 상수 ----
constexpr double kMaxDtScreenSaver = 0.1;
constexpr double kMaxDtPreview = 1.0;
constexpr double kCPUTargetFrameTime = 1.0 / 30.0;
constexpr double kInterpFrameTime = 1.0 / 60.0;

// 미리보기 모드
constexpr DWORD  kPreviewRenderMs = 500;
constexpr int    kDefaultPreviewW = 152;
constexpr int    kDefaultPreviewH = 112;

// 레지스트리 상수
static constexpr const wchar_t* kDesktopRegPath = L"Control Panel\\Desktop";
static constexpr const wchar_t* kScrRegKey = L"SCRNSAVE.EXE";

// UAC 권한 상승 후 포그라운드 잠금을 우회하여 MessageBox를 앞으로 표시
static int ForegroundMessageBox(const wchar_t* text, const wchar_t* caption, UINT type) {
    keybd_event(VK_MENU, 0, 0, 0);
    keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
    return MessageBoxW(nullptr, text, caption, type | MB_TOPMOST | MB_SETFOREGROUND);
}

// ---- 생성/소멸 ----

ScreenSaverEngine::ScreenSaverEngine(const AppDescriptor& desc,
                                     std::unique_ptr<IScreenSaverContent> content)
    : desc_(desc), content_(std::move(content)) {}

ScreenSaverEngine::~ScreenSaverEngine() = default;

// ---- 커맨드라인 파싱 헬퍼 ----

// GetCommandLineW()에서 프로그램 경로를 건너뛰고 인자 부분만 추출
static const wchar_t* SkipProgramPath(const wchar_t* cmdLine) {
    if (!cmdLine) return L"";
    const wchar_t* p = cmdLine;
    while (*p == L' ') p++;
    if (*p == L'"') {
        p++;
        while (*p && *p != L'"') p++;
        if (*p == L'"') p++;
    } else {
        while (*p && *p != L' ') p++;
    }
    while (*p == L' ') p++;
    return p;
}

// /lang:xx 추출
static const wchar_t* ExtractLangArg(const wchar_t* args) {
    const wchar_t* found = StrStrIW(args, L"/lang:");
    if (!found) found = StrStrIW(args, L"-lang:");
    if (!found) return nullptr;
    return found + 6;
}

// /screenshot:interval[:folder] 추출
static bool ExtractScreenshotArg(const wchar_t* args, ScreenshotConfig& cfg) {
    const wchar_t* found = StrStrIW(args, L"/screenshot:");
    if (!found) found = StrStrIW(args, L"-screenshot:");
    if (!found) return false;

    const wchar_t* p = found + 12;
    int interval = 0;
    while (*p >= L'0' && *p <= L'9') {
        interval = interval * 10 + (*p - L'0');
        p++;
    }
    if (interval <= 0) return false;
    cfg.intervalSec = interval;

    if (*p == L':') {
        p++;
        if (*p == L'"') {
            p++;
            int i = 0;
            while (*p && *p != L'"' && i < MAX_PATH - 1)
                cfg.folder[i++] = *p++;
            cfg.folder[i] = L'\0';
        } else {
            int i = 0;
            while (*p && *p != L' ' && i < MAX_PATH - 1)
                cfg.folder[i++] = *p++;
            cfg.folder[i] = L'\0';
        }
    }

    // 폴더 미지정 시 기본 경로는 Run()에서 F2 경로로 설정
    if (cfg.folder[0] != L'\0') {
        CreateDirectoryW(cfg.folder, nullptr);
    }
    return true;
}

// 실행 모드 파싱
static AppMode ParseMode(const wchar_t* cmdLine, HWND& previewHwnd, int& renderOverride) {
    previewHwnd = nullptr;
    renderOverride = -1;

    if (!cmdLine || !*cmdLine)
        return AppMode::Configure;

    AppMode result = AppMode::Configure;
    const wchar_t* p = cmdLine;

    while (*p) {
        while (*p == L' ') p++;
        if (!*p) break;

        if (*p == L'/' || *p == L'-') {
            p++;
            wchar_t ch = static_cast<wchar_t>(towlower(*p));

            if (ch == L'c' && towlower(p[1]) == L'p' && towlower(p[2]) == L'u') {
                renderOverride = 1;
                p += 3;
                continue;
            }
            if (ch == L'g' && towlower(p[1]) == L'p' && towlower(p[2]) == L'u') {
                renderOverride = 0;
                p += 3;
                continue;
            }
            if (_wcsnicmp(p, L"lang:", 5) == 0) {
                p += 5;
                while (*p && *p != L' ') p++;
                continue;
            }
            if (_wcsnicmp(p, L"screenshot:", 11) == 0) {
                p += 11;
                while (*p && *p != L' ') p++;
                if (*p == L'"') {
                    p++;
                    while (*p && *p != L'"') p++;
                    if (*p == L'"') p++;
                }
                continue;
            }

            switch (ch) {
            case L's':
            case L'w':
                result = AppMode::ScreenSaver;
                p++;
                break;
            case L'c':
                result = AppMode::Configure;
                p++;
                break;
            case L'p': {
                p++;
                while (*p == L' ' || *p == L':') p++;
                if (*p >= L'0' && *p <= L'9') {
                    uintptr_t hwndVal = 0;
                    while (*p >= L'0' && *p <= L'9') {
                        hwndVal = hwndVal * 10 + (*p - L'0');
                        p++;
                    }
                    previewHwnd = reinterpret_cast<HWND>(hwndVal);
                }
                result = AppMode::Preview;
                break;
            }
            default:
                p++;
                break;
            }
        } else {
            while (*p && *p != L' ') p++;
        }
    }

    return result;
}

// ---- 파일 버전 유틸리티 ----

struct FileVersion {
    DWORD major;
    DWORD minor;
    DWORD patch;
};

static bool GetFileVer(const wchar_t* path, FileVersion& ver) {
    DWORD dummy = 0;
    DWORD size = GetFileVersionInfoSizeW(path, &dummy);
    if (size == 0) return false;

    BYTE* buf = new (std::nothrow) BYTE[size];
    if (!buf) return false;

    bool ok = false;
    if (GetFileVersionInfoW(path, 0, size, buf)) {
        VS_FIXEDFILEINFO* ffi = nullptr;
        UINT ffiLen = 0;
        if (VerQueryValueW(buf, L"\\", reinterpret_cast<void**>(&ffi), &ffiLen)) {
            ver.major = HIWORD(ffi->dwFileVersionMS);
            ver.minor = LOWORD(ffi->dwFileVersionMS);
            ver.patch = HIWORD(ffi->dwFileVersionLS);
            ok = true;
        }
    }

    delete[] buf;
    return ok;
}

static int CompareVersions(const FileVersion& a, const FileVersion& b) {
    if (a.major != b.major) return (a.major > b.major) ? 1 : -1;
    if (a.minor != b.minor) return (a.minor > b.minor) ? 1 : -1;
    if (a.patch != b.patch) return (a.patch > b.patch) ? 1 : -1;
    return 0;
}

// ---- 권한 승격 ----

bool ScreenSaverEngine::IsElevated() {
    BOOL elevated = FALSE;
    HANDLE token = nullptr;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        TOKEN_ELEVATION elev{};
        DWORD size = sizeof(elev);
        if (GetTokenInformation(token, TokenElevation, &elev, sizeof(elev), &size))
            elevated = elev.TokenIsElevated;
        CloseHandle(token);
    }
    return elevated != FALSE;
}

bool ScreenSaverEngine::RelaunchElevated(const wchar_t* args) {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);

    SHELLEXECUTEINFOW sei{};
    sei.cbSize = sizeof(sei);
    sei.lpVerb = L"runas";
    sei.lpFile = exePath;
    sei.lpParameters = args;
    sei.nShow = SW_SHOWNORMAL;
    return ShellExecuteExW(&sei) != FALSE;
}

int ScreenSaverEngine::ElevateIfNeeded(const wchar_t* args) {
    if (IsElevated()) return -1;
    if (RelaunchElevated(args)) return 0;
    ForegroundMessageBox(TR(Str::AdminRequired),
                desc_.appTitle, MB_ICONERROR | MB_OK);
    return 1;
}

// ---- 설치 ----

// 기존 프로세스 종료 (설정 창 등이 파일을 잠그는 것 방지)
// scrName: "FractalSaver.scr" -> "FractalSaver.scr"과 "FractalSaver.exe" 모두 종료
static void KillRunningScr(const wchar_t* scrName) {
    if (!scrName) return;

    // 확장자를 .exe로 바꾼 이름도 생성
    wchar_t exeName[MAX_PATH];
    wcscpy_s(exeName, scrName);
    wchar_t* dot = wcsrchr(exeName, L'.');
    if (dot)
        wcscpy_s(dot, exeName + MAX_PATH - dot, L".exe");

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return;

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);
    DWORD selfPid = GetCurrentProcessId();

    if (Process32FirstW(snap, &pe)) {
        do {
            if (pe.th32ProcessID != selfPid &&
                (_wcsicmp(pe.szExeFile, scrName) == 0 ||
                 _wcsicmp(pe.szExeFile, exeName) == 0)) {
                HANDLE hp = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                if (hp) {
                    TerminateProcess(hp, 0);
                    CloseHandle(hp);
                }
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
}

int ScreenSaverEngine::DoInstall() {
    // 기존 .scr 프로세스 종료 (파일 잠금 해제)
    KillRunningScr(desc_.scrName);
    if (desc_.otherScrName)
        KillRunningScr(desc_.otherScrName);

    wchar_t selfPath[MAX_PATH];
    GetModuleFileNameW(nullptr, selfPath, MAX_PATH);

    wchar_t sys32[MAX_PATH];
    GetSystemDirectoryW(sys32, MAX_PATH);

    wchar_t dstPath[MAX_PATH];
    _snwprintf_s(dstPath, MAX_PATH, _TRUNCATE, L"%s\\%s", sys32, desc_.scrName);

    // 상대 버전 제거
    if (desc_.otherScrName) {
        wchar_t otherPath[MAX_PATH];
        _snwprintf_s(otherPath, MAX_PATH, _TRUNCATE, L"%s\\%s", sys32, desc_.otherScrName);
        if (PathFileExistsW(otherPath))
            DeleteFileW(otherPath);
    }

    if (!CopyFileW(selfPath, dstPath, FALSE)) {
        DWORD err = GetLastError();
        wchar_t msg[512];
        _snwprintf_s(msg, 512, _TRUNCATE, TR(Str::InstallFailed), err);
        ForegroundMessageBox(msg, desc_.appTitle, MB_ICONERROR | MB_OK);
        return 1;
    }

    // 레지스트리에 활성 스크린세이버 등록
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kDesktopRegPath, 0,
                      KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, kScrRegKey, 0, REG_SZ,
            reinterpret_cast<const BYTE*>(dstPath),
            static_cast<DWORD>((wcslen(dstPath) + 1) * sizeof(wchar_t)));
        const wchar_t* one = L"1";
        RegSetValueExW(hKey, L"ScreenSaveActive", 0, REG_SZ,
            reinterpret_cast<const BYTE*>(one), 2 * sizeof(wchar_t));
        RegCloseKey(hKey);
    }

    SystemParametersInfoW(SPI_SETSCREENSAVEACTIVE, TRUE, nullptr,
                          SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);

    if (!silent_) {
        wchar_t msg[512];
        _snwprintf_s(msg, 512, _TRUNCATE, TR(Str::InstallSuccess), desc_.appTitle);
        ForegroundMessageBox(msg, desc_.appTitle, MB_ICONINFORMATION | MB_OK);

        ShellExecuteW(nullptr, L"open", L"control.exe",
            L"desk.cpl,,@ScreenSaver", nullptr, SW_SHOWNORMAL);
    }

    return 0;
}

// ---- 제거 ----

int ScreenSaverEngine::DoUninstall() {
    wchar_t sys32[MAX_PATH];
    GetSystemDirectoryW(sys32, MAX_PATH);

    wchar_t scrPath[MAX_PATH];
    _snwprintf_s(scrPath, MAX_PATH, _TRUNCATE, L"%s\\%s", sys32, desc_.scrName);

    if (!silent_) {
        wchar_t msg[512];
        _snwprintf_s(msg, 512, _TRUNCATE, TR(Str::UninstallConfirm), desc_.appTitle);
        int result = ForegroundMessageBox(msg,
            desc_.appTitle, MB_ICONQUESTION | MB_YESNO);
        if (result != IDYES) return 0;
    }

    // 레지스트리 정리
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kDesktopRegPath, 0,
                      KEY_SET_VALUE | KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS) {
        wchar_t val[MAX_PATH]{};
        DWORD valSize = sizeof(val);
        DWORD type = 0;
        if (RegQueryValueExW(hKey, kScrRegKey, nullptr, &type,
                             reinterpret_cast<BYTE*>(val), &valSize) == ERROR_SUCCESS) {
            bool isOurs = StrStrIW(val, desc_.scrName) != nullptr;
            if (!isOurs && desc_.otherScrName)
                isOurs = StrStrIW(val, desc_.otherScrName) != nullptr;
            if (isOurs)
                RegDeleteValueW(hKey, kScrRegKey);
        }
        RegCloseKey(hKey);
    }

    if (desc_.registryKey)
        RegDeleteKeyW(HKEY_CURRENT_USER, desc_.registryKey);

    SystemParametersInfoW(SPI_SETSCREENSAVEACTIVE, FALSE, nullptr,
                          SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);

    HWND hScrWnd = FindWindowW(nullptr, desc_.scrName);
    if (hScrWnd)
        PostMessageW(hScrWnd, WM_CLOSE, 0, 0);
    Sleep(500);

    // .scr 파일 삭제
    bool deleted = true;
    if (PathFileExistsW(scrPath)) {
        if (!DeleteFileW(scrPath)) {
            Sleep(1000);
            if (!DeleteFileW(scrPath))
                deleted = false;
        }
    }
    if (desc_.otherScrName) {
        wchar_t otherPath[MAX_PATH];
        _snwprintf_s(otherPath, MAX_PATH, _TRUNCATE, L"%s\\%s", sys32, desc_.otherScrName);
        if (PathFileExistsW(otherPath)) {
            if (!DeleteFileW(otherPath)) {
                Sleep(1000);
                if (!DeleteFileW(otherPath))
                    deleted = false;
            }
        }
    }

    if (!deleted) {
        MoveFileExW(scrPath, nullptr, MOVEFILE_DELAY_UNTIL_REBOOT);
        if (!silent_) {
            wchar_t msg[512];
            _snwprintf_s(msg, 512, _TRUNCATE, TR(Str::UninstallDelayed), desc_.appTitle);
            ForegroundMessageBox(msg, desc_.appTitle, MB_ICONINFORMATION | MB_OK);
        }
        return 0;
    }

    if (!silent_) {
        wchar_t msg[512];
        _snwprintf_s(msg, 512, _TRUNCATE, TR(Str::UninstallComplete), desc_.appTitle);
        ForegroundMessageBox(msg, desc_.appTitle, MB_ICONINFORMATION | MB_OK);
    }
    return 0;
}

// ---- 런처 다이얼로그 ----

static constexpr int kBtnInstall = 1001;
static constexpr int kBtnPreview = 1002;
static constexpr int kBtnUninstall = 1003;

static HRESULT CALLBACK LauncherDialogCallback(
    HWND, UINT msg, WPARAM wParam, LPARAM, LONG_PTR)
{
    if (msg == TDN_BUTTON_CLICKED && static_cast<int>(wParam) == kBtnPreview) {
        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        ShellExecuteW(nullptr, nullptr, exePath, L"/s", nullptr, SW_SHOWNORMAL);
        return S_FALSE;
    }
    return S_OK;
}

int ScreenSaverEngine::ShowLauncher(HINSTANCE) {
    wchar_t selfPath[MAX_PATH];
    GetModuleFileNameW(nullptr, selfPath, MAX_PATH);

    wchar_t sys32[MAX_PATH];
    GetSystemDirectoryW(sys32, MAX_PATH);

    wchar_t scrPath[MAX_PATH];
    _snwprintf_s(scrPath, MAX_PATH, _TRUNCATE, L"%s\\%s", sys32, desc_.scrName);
    bool installed = PathFileExistsW(scrPath) != FALSE;

    bool otherInstalled = false;
    wchar_t otherScrPath[MAX_PATH] = {};
    if (desc_.otherScrName) {
        _snwprintf_s(otherScrPath, MAX_PATH, _TRUNCATE, L"%s\\%s", sys32, desc_.otherScrName);
        otherInstalled = PathFileExistsW(otherScrPath) != FALSE;
    }

    bool anyInstalled = installed || otherInstalled;

    wchar_t contentText[512] = {};
    wchar_t installBtnText[256] = {};

    if (otherInstalled) {
        FileVersion otherVer = {};
        GetFileVer(otherScrPath, otherVer);
        wchar_t verStr[32];
        _snwprintf_s(verStr, 32, _TRUNCATE, L"%u.%u.%u",
            otherVer.major, otherVer.minor, otherVer.patch);
        _snwprintf_s(contentText, _countof(contentText), _TRUNCATE,
            TR(Str::CrossEdReplace),
            desc_.otherAppTitle, verStr, desc_.appTitle);
        _snwprintf_s(installBtnText, _countof(installBtnText), _TRUNCATE,
            TR(Str::CrossEdReplaceBtn),
            desc_.otherAppTitle, desc_.appTitle);
    } else if (installed) {
        FileVersion installedVer = {};
        FileVersion currentVer = {};
        GetFileVer(scrPath, installedVer);
        GetFileVer(selfPath, currentVer);
        wchar_t verStr[32];
        _snwprintf_s(verStr, 32, _TRUNCATE, L"%u.%u.%u",
            installedVer.major, installedVer.minor, installedVer.patch);

        int cmp = CompareVersions(installedVer, currentVer);
        if (cmp == 0) {
            _snwprintf_s(contentText, _countof(contentText), _TRUNCATE,
                TR(Str::VerReinstall), verStr);
            wcscpy_s(installBtnText, TR(Str::ReinstallBtn));
        } else if (cmp < 0) {
            _snwprintf_s(contentText, _countof(contentText), _TRUNCATE,
                TR(Str::VerUpgrade), verStr);
            wcscpy_s(installBtnText, TR(Str::UpgradeBtn));
        } else {
            _snwprintf_s(contentText, _countof(contentText), _TRUNCATE,
                TR(Str::VerDowngrade), verStr);
            wcscpy_s(installBtnText, TR(Str::DowngradeBtn));
        }
    } else {
        wcscpy_s(contentText, TR(Str::ChooseAction));
        wcscpy_s(installBtnText, TR(Str::InstallBtn));
    }

    wchar_t previewBtnText[256];
    wcscpy_s(previewBtnText, TR(Str::PreviewBtn));
    wchar_t uninstallBtnText[256];
    wcscpy_s(uninstallBtnText, TR(Str::UninstallBtn));

    const TASKDIALOG_BUTTON btnsWithUninstall[] = {
        { kBtnInstall,   installBtnText },
        { kBtnPreview,   previewBtnText },
        { kBtnUninstall, uninstallBtnText },
    };
    const TASKDIALOG_BUTTON btnsNoUninstall[] = {
        { kBtnInstall,   installBtnText },
        { kBtnPreview,   previewBtnText },
    };

    TASKDIALOGCONFIG tdc = {};
    tdc.cbSize = sizeof(tdc);
    tdc.dwFlags = TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION;
    tdc.pszWindowTitle = desc_.appTitle;
    tdc.pszMainIcon = TD_INFORMATION_ICON;
    tdc.pszMainInstruction = desc_.appTitle;
    tdc.pszContent = contentText;
    tdc.pButtons = anyInstalled ? btnsWithUninstall : btnsNoUninstall;
    tdc.cButtons = anyInstalled ? _countof(btnsWithUninstall) : _countof(btnsNoUninstall);
    tdc.nDefaultButton = kBtnInstall;
    tdc.pfCallback = LauncherDialogCallback;

    int clicked = 0;
    if (FAILED(TaskDialogIndirect(&tdc, &clicked, nullptr, nullptr)))
        return 1;

    if (clicked == kBtnInstall) {
        int r = ElevateIfNeeded(L"/install");
        return (r >= 0) ? r : DoInstall();
    }
    if (clicked == kBtnUninstall) {
        int r = ElevateIfNeeded(L"/u");
        return (r >= 0) ? r : DoUninstall();
    }
    return 0;
}

// ---- 메인 엔트리 ----

int ScreenSaverEngine::Run(HINSTANCE hInst) {
    SetProcessDPIAware();

    const wchar_t* args = SkipProgramPath(GetCommandLineW());

    // I18N 초기화
    Localization::Init(ExtractLangArg(args));

    // 레지스트리 키 경로 설정
    if (desc_.registryKey)
        Registry::SetKeyPath(desc_.registryKey);

    // /silent 플래그
    if (StrStrIW(args, L"/silent") || StrStrIW(args, L"-silent"))
        silent_ = true;

    // 설정 로드
    Registry::LoadFrameworkSettings(settings_);
    content_->LoadSettings();

    // 설치/제거 플래그 감지
    const wchar_t* p = args;
    while (*p == L' ') p++;

    if (*p == L'/' || *p == L'-') {
        if (_wcsnicmp(p + 1, L"install", 7) == 0) {
            const wchar_t* elevateArgs = silent_ ? L"/install /silent" : L"/install";
            int r = ElevateIfNeeded(elevateArgs);
            return (r >= 0) ? r : DoInstall();
        }
        if (towlower(p[1]) == L'u' && (p[2] == L'\0' || p[2] == L' ')) {
            const wchar_t* elevateArgs = silent_ ? L"/u /silent" : L"/u";
            int r = ElevateIfNeeded(elevateArgs);
            return (r >= 0) ? r : DoUninstall();
        }
    }

    // .exe 확장자 + 인자 없음 -> 런처 다이얼로그
    if (*p == L'\0') {
        wchar_t selfPath[MAX_PATH];
        GetModuleFileNameW(nullptr, selfPath, MAX_PATH);
        if (_wcsicmp(PathFindExtensionW(selfPath), L".exe") == 0) {
            return ShowLauncher(hInst);
        }
    }

    // 다중 모니터 에뮬레이션 (/emu 또는 /emu:N)
    {
        const wchar_t* emuFound = StrStrIW(args, L"/emu");
        if (!emuFound) emuFound = StrStrIW(args, L"-emu");
        if (emuFound) {
            const wchar_t* ep = emuFound + 4;
            if (*ep == L':') {
                ep++;
                int n = 0;
                while (*ep >= L'0' && *ep <= L'9') {
                    n = n * 10 + (*ep - L'0');
                    ep++;
                }
                emuMonitors_ = (n >= 2) ? n : 2;
            } else {
                emuMonitors_ = 2;
            }
        }
    }

    // 스크린세이버 모드
    HWND previewHwnd = nullptr;
    int renderOverride = -1;
    auto mode = ParseMode(args, previewHwnd, renderOverride);

    // 렌더 모드 오버라이드 적용
    if (renderOverride == 1) settings_.forceCPU = true;
    else if (renderOverride == 0) settings_.forceCPU = false;

    // 스크린샷 설정
    ScreenshotConfig ssCfg;
    bool hasScreenshot = ExtractScreenshotArg(args, ssCfg);

    // 자동 스크린샷 기본 폴더: F2 수동 캡처와 동일 경로
    if (hasScreenshot && ssCfg.folder[0] == L'\0') {
        wchar_t userProfile[MAX_PATH];
        if (GetEnvironmentVariableW(L"USERPROFILE", userProfile, MAX_PATH)) {
            wchar_t parentDir[MAX_PATH];
            _snwprintf_s(parentDir, MAX_PATH, _TRUNCATE,
                         L"%s\\%s", userProfile, desc_.appTitle);
            CreateDirectoryW(parentDir, nullptr);
            _snwprintf_s(ssCfg.folder, MAX_PATH, _TRUNCATE,
                         L"%s\\screenshot", parentDir);
            CreateDirectoryW(ssCfg.folder, nullptr);
        }
    }

    // /emu 단독 사용 시 스크린세이버 모드로 전환
    if (emuMonitors_ > 0 && mode == AppMode::Configure)
        mode = AppMode::ScreenSaver;

    switch (mode) {
    case AppMode::ScreenSaver:
        return RunScreenSaver(hInst, hasScreenshot ? &ssCfg : nullptr);
    case AppMode::Configure:
        return RunConfigure(hInst);
    case AppMode::Preview:
        return RunPreview(hInst, previewHwnd);
    }
    return 0;
}

// ---- 스크린세이버 모드 ----

int ScreenSaverEngine::RunScreenSaver(HINSTANCE hInst, const ScreenshotConfig* screenshot) {
    ScreenSaverWindow window;
    if (!window.Init(hInst, false, emuMonitors_)) return 1;

    int screenW = window.GetWidth();
    int screenH = window.GetHeight();

    int renderW = 0;
    int renderH = 0;
    content_->GetRenderSize(screenW, screenH, renderW, renderH);

    content_->Init(renderW, renderH, settings_.forceCPU);

    bool showContent = settings_.showContent;
    bool showOverlay = desc_.hasOverlay && settings_.showOverlay;
    bool showClock = desc_.hasClock && settings_.showClock;

    // 시스템 정보 수집 (오버레이 사용 시에만)
    SystemInfo sysInfo;
    if (desc_.hasOverlay) sysInfo.Collect();

    // 오버레이 초기화
    TextOverlay textOverlay;
    if (desc_.hasOverlay)
        textOverlay.Init(renderW, renderH, screenW, screenH,
                         content_->GetSurfaceDC(), sysInfo, content_->IsLite());

    ClockOverlay clockOverlay;
    if (showClock) clockOverlay.Init(renderW, renderH);

    // FPS/CPU 측정
    double fps = 0.0;
    LARGE_INTEGER fpsFreq, fpsLast, fpsCurr;
    QueryPerformanceFrequency(&fpsFreq);
    QueryPerformanceCounter(&fpsLast);
    int frameCount = 0;

    double cpuUsage = 0.0;
    FILETIME prevIdleTime{}, prevKernelTime{}, prevUserTime{};
    GetSystemTimes(&prevIdleTime, &prevKernelTime, &prevUserTime);

    // 표시 중인 페이드 알파 (오버레이 밝기에 사용)
    float displayFadeAlpha = 0.0f;

    // 오버레이 동적 라인 버퍼
    wchar_t dynLine[256] = {};

    // 렌더 콜백
    window.SetRenderCallback([&](HDC hdc, int w, int h) {
        HDC surfDC = content_->GetSurfaceDC();

        // 콘텐츠 비활성 시 검은 배경으로 클리어
        if (!showContent) {
            RECT r = {0, 0, renderW, renderH};
            FillRect(surfDC, &r, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
        }

        // 보조 모니터에 먼저 복사 (오버레이/시계 없는 순수 프랙탈)
        window.UpdateMirrorWindows(surfDC, renderW, renderH);

        // 콘텐츠 합성 결과를 surfDC에 출력 (hdc가 아닌 surfDC)
        // BlitTo가 자체 줌 보정 + 그라데이션 합성 수행
        if (showContent) {
            content_->BlitTo(surfDC, renderW, renderH);
        }

        // 오버레이/시계를 surfDC 위에 합성
        if (showClock) clockOverlay.Render(surfDC, renderW, renderH);
        if (desc_.hasOverlay) {
            if (showOverlay) {
                if (showContent)
                    content_->FormatOverlayLine(dynLine, 256, fps);
                else
                    dynLine[0] = L'\0';
                textOverlay.Render(surfDC, displayFadeAlpha, dynLine, cpuUsage);
            } else {
                textOverlay.RenderHelpLine(surfDC, displayFadeAlpha);
            }
        }

        // surfDC(합성 + 오버레이)를 단일 StretchBlt로 화면 출력 (깜빡임 방지)
        content_->GetSurface().StretchBlitTo(hdc, 0, 0, w, h);
    });

    // 시스템 스크린세이버/절전 모드 억제 (수동 /s 실행 시 시스템 세이버가 뜨지 않도록)
    SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED);

    // 첫 렌더 시작
    bool renderInProgress = false;
    float renderStartFade = 0.0f;
    if (showContent) {
        content_->Update(0.0);
        content_->BeginRender();
        renderInProgress = true;
        renderStartFade = content_->GetFadeAlpha();
    } else {
        window.RequestRedraw();
    }

    LARGE_INTEGER freq, prevTime, currTime;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&prevTime);

    LARGE_INTEGER lastFrameTime;
    QueryPerformanceCounter(&lastFrameTime);
    bool cpuFrameCap = !content_->IsUsingGPU();

    // 시간 보간: 비동기 렌더링 중 이전 프레임 확대
    HDC prevFrameDC = nullptr;
    HBITMAP prevFrameBmp = nullptr;
    HGDIOBJ prevFrameOldBmp = nullptr;
    bool hasPrevFrame = false;
    bool fadeFirstRenderDone = false;
    IScreenSaverContent::InterpInfo prevInterpInfo{};
    LARGE_INTEGER lastInterpTime;
    QueryPerformanceCounter(&lastInterpTime);

    if (content_->SupportsInterpolation()) {
        BITMAPINFO bi = {};
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = renderW;
        bi.bmiHeader.biHeight = -renderH;
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        bi.bmiHeader.biCompression = BI_RGB;
        void* bits = nullptr;
        prevFrameBmp = CreateDIBSection(nullptr, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
        prevFrameDC = CreateCompatibleDC(nullptr);
        prevFrameOldBmp = SelectObject(prevFrameDC, prevFrameBmp);
    }

    // 스크린샷 상태
    bool screenshotActive = screenshot && screenshot->intervalSec > 0;
    LARGE_INTEGER lastScreenshotTime;
    QueryPerformanceCounter(&lastScreenshotTime);
    int screenshotSeq = 0;

    MSG msg{};
    bool running = true;

    while (running) {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        if (!running) break;

        // F1: 설정 다이얼로그
        if (window.IsConfigRequested()) {
            window.ClearConfigRequest();

            if (renderInProgress && content_->IsRendering()) {
                while (!content_->IsRenderComplete())
                    Sleep(1);
                content_->FinalizeRender();
                renderInProgress = false;
            }

            ShowCursor(TRUE);
            window.SetConfigDialogOpen(true);

            FrameworkSettings oldSettings = settings_;
            bool accepted = ConfigDialog::Show(hInst, window.GetHwnd(),
                                               desc_, settings_, content_.get());

            window.SetConfigDialogOpen(false);
            window.ResetInputTracking();
            ShowCursor(FALSE);

            if (accepted) {
                if (settings_.forceCPU != oldSettings.forceCPU) {
                    if (content_->ApplyFrameworkSettings(renderW, renderH, settings_.forceCPU)) {
                        cpuFrameCap = !content_->IsUsingGPU();
                        hasPrevFrame = false;
                    }
                }
                showContent = settings_.showContent;
                showOverlay = desc_.hasOverlay && settings_.showOverlay;
                bool wasShowClock = showClock;
                showClock = desc_.hasClock && settings_.showClock;
                if (showClock && !wasShowClock) {
                    clockOverlay.Init(renderW, renderH);
                }
            }

            window.ClearConfigRequest();

            if (showContent) {
                content_->BeginRender();
                renderInProgress = true;
                renderStartFade = content_->GetFadeAlpha();
            } else {
                window.RequestRedraw();
            }
            QueryPerformanceCounter(&prevTime);
        }

        // 시간 단계
        QueryPerformanceCounter(&currTime);
        double dt = static_cast<double>(currTime.QuadPart - prevTime.QuadPart) / freq.QuadPart;
        prevTime = currTime;
        if (dt > kMaxDtScreenSaver) dt = kMaxDtScreenSaver;

        // CPU 사용률 측정 (콘텐츠 비활성 시에도 갱신)
        QueryPerformanceCounter(&fpsCurr);
        double cpuElapsed = static_cast<double>(fpsCurr.QuadPart - fpsLast.QuadPart) / fpsFreq.QuadPart;
        if (cpuElapsed >= 1.0) {
            FILETIME curIdleTime, curKernelTime, curUserTime;
            if (GetSystemTimes(&curIdleTime, &curKernelTime, &curUserTime)) {
                auto ToU64 = [](const FILETIME& ft) -> ULONGLONG {
                    return (static_cast<ULONGLONG>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
                };
                ULONGLONG idle = ToU64(curIdleTime) - ToU64(prevIdleTime);
                ULONGLONG kernel = ToU64(curKernelTime) - ToU64(prevKernelTime);
                ULONGLONG user = ToU64(curUserTime) - ToU64(prevUserTime);
                ULONGLONG total = kernel + user;
                cpuUsage = (total > 0) ? (1.0 - static_cast<double>(idle) / total) * 100.0 : 0.0;
                prevIdleTime = curIdleTime;
                prevKernelTime = curKernelTime;
                prevUserTime = curUserTime;
            }
            if (!showContent) {
                fps = 0.0;
                frameCount = 0;
                fpsLast = fpsCurr;
            }
        }

        // 콘텐츠 비활성 시 시계/오버레이만 갱신
        if (!showContent) {
            if (renderInProgress) {
                if (content_->IsRendering()) {
                    while (!content_->IsRenderComplete())
                        Sleep(1);
                    content_->FinalizeRender();
                }
                renderInProgress = false;
            }
            if (showClock) {
                window.RequestRedraw();
                Sleep(33);
            } else {
                Sleep(100);
            }
            continue;
        }

        // 렌더 완료 확인
        if (renderInProgress && content_->IsRenderComplete()) {
            content_->FinalizeRender();

            // 보간용 이전 프레임 백업
            if (content_->SupportsInterpolation() && prevFrameDC) {
                BitBlt(prevFrameDC, 0, 0, renderW, renderH,
                       content_->GetSurfaceDC(), 0, 0, SRCCOPY);
                prevInterpInfo = content_->GetInterpInfo();
            }

            bool wasFirstFrame = !hasPrevFrame;
            hasPrevFrame = true;
            renderInProgress = false;
            float curFade = content_->GetFadeAlpha();
            if (curFade < 1.0f) fadeFirstRenderDone = true;

            // 보간 지원 시: 보간 코드가 화면 갱신을 관리 (중복 호출 시 진동 발생)
            // 보간 미지원 시: 렌더 완료마다 직접 화면 갱신 (유일한 갱신 경로)
            if (content_->IsUsingGPU() || wasFirstFrame || curFade < 1.0f
                || !content_->SupportsInterpolation()) {
                displayFadeAlpha = renderStartFade;
                window.RequestRedraw();
            }

            // FPS 측정 (CPU 사용률은 루프 상단에서 갱신)
            frameCount++;
            if (cpuElapsed >= 1.0) {
                fps = frameCount / cpuElapsed;
                frameCount = 0;
                fpsLast = fpsCurr;
            }

            // 스크린샷 저장
            if (screenshotActive && content_->GetFadeAlpha() >= 1.0f) {
                QueryPerformanceCounter(&currTime);
                double sinceSS = static_cast<double>(currTime.QuadPart - lastScreenshotTime.QuadPart) / freq.QuadPart;
                if (sinceSS >= screenshot->intervalSec) {
                    lastScreenshotTime = currTime;
                    wchar_t ssPath[MAX_PATH];
                    _snwprintf_s(ssPath, MAX_PATH, _TRUNCATE,
                                 L"%s\\fractal_%04d.png", screenshot->folder, screenshotSeq++);
                    content_->GetSurface().SavePNG(ssPath);
                }
            }

            // 스타일 순회 완료 시 종료
            if (screenshotActive && content_->GetCycleCount() >= 1) {
                running = false;
                break;
            }
        }

        // F2: 수동 스크린샷 저장
        if (window.IsScreenshotRequested()) {
            window.ClearScreenshotRequest();
            // %USERPROFILE%\{appTitle}\screenshot 폴더 생성
            wchar_t userProfile[MAX_PATH];
            if (GetEnvironmentVariableW(L"USERPROFILE", userProfile, MAX_PATH)) {
                wchar_t parentDir[MAX_PATH];
                _snwprintf_s(parentDir, MAX_PATH, _TRUNCATE,
                             L"%s\\%s", userProfile, desc_.appTitle);
                CreateDirectoryW(parentDir, nullptr);
                wchar_t ssDir[MAX_PATH];
                _snwprintf_s(ssDir, MAX_PATH, _TRUNCATE,
                             L"%s\\screenshot", parentDir);
                CreateDirectoryW(ssDir, nullptr);
                // 콘텐츠가 파일명 결정
                wchar_t fileName[MAX_PATH];
                content_->FormatScreenshotName(fileName, MAX_PATH);
                wchar_t ssPath[MAX_PATH];
                _snwprintf_s(ssPath, MAX_PATH, _TRUNCATE,
                             L"%s\\%s.png", ssDir, fileName);
                content_->GetSurface().SavePNG(ssPath);
            }
        }

        // 애니메이션 갱신
        float curFade = content_->GetFadeAlpha();
        if (curFade >= 1.0f || !renderInProgress) {
            float prevFade = curFade;
            content_->Update(dt);
            curFade = content_->GetFadeAlpha();

            if (prevFade <= 0.0f && curFade > 0.0f)
                fadeFirstRenderDone = false;

            // FadingIn -> Visible 전환 시 전체 밝기로 갱신
            if (prevFade < 1.0f && curFade >= 1.0f && fadeFirstRenderDone && content_->CanReapplyFade()) {
                content_->ReapplyFade(1.0f);
                if (content_->SupportsInterpolation() && prevFrameDC) {
                    BitBlt(prevFrameDC, 0, 0, renderW, renderH,
                           content_->GetSurfaceDC(), 0, 0, SRCCOPY);
                    prevInterpInfo = content_->GetInterpInfo();
                }
                displayFadeAlpha = curFade;
                window.RequestRedraw();
            }
        }

        // 시간 보간
        if (content_->SupportsInterpolation() && renderInProgress && hasPrevFrame && curFade >= 1.0f) {
            QueryPerformanceCounter(&currTime);
            double sinceLast = static_cast<double>(currTime.QuadPart - lastInterpTime.QuadPart) / freq.QuadPart;
            if (sinceLast >= kInterpFrameTime) {
                lastInterpTime = currTime;

                // BlitTo가 줌 보정을 담당하므로 StretchBlt 불필요
                // RequestRedraw만 트리거하여 BlitTo가 최신 줌 상태 반영
                auto info = content_->GetInterpInfo();
                double zoomRatio = prevInterpInfo.prevScale / info.currScale;
                if (zoomRatio > 1.001) {
                    displayFadeAlpha = curFade;
                    window.RequestRedraw();
                }
            }
        }

        // 새 렌더 시작
        if (!renderInProgress) {
            if (cpuFrameCap) {
                QueryPerformanceCounter(&currTime);
                double sinceLastFrame = static_cast<double>(currTime.QuadPart - lastFrameTime.QuadPart) / freq.QuadPart;
                if (sinceLastFrame < kCPUTargetFrameTime) {
                    Sleep(1);
                    continue;
                }
                QueryPerformanceCounter(&lastFrameTime);
            }

            curFade = content_->GetFadeAlpha();
            if (curFade < 1.0f && fadeFirstRenderDone && content_->CanReapplyFade()) {
                content_->ReapplyFade(curFade);
                displayFadeAlpha = curFade;
                window.RequestRedraw();
            } else {
                renderStartFade = curFade;
                content_->BeginRender();
                renderInProgress = true;
            }
        }

        Sleep(1);
    }

    // 진행 중인 렌더 완료 대기
    if (renderInProgress && content_->IsRendering()) {
        while (!content_->IsRenderComplete())
            Sleep(1);
        content_->FinalizeRender();
    }

    // 시스템 스크린세이버/절전 모드 억제 해제
    SetThreadExecutionState(ES_CONTINUOUS);

    // 보간 버퍼 정리
    if (prevFrameDC) {
        SelectObject(prevFrameDC, prevFrameOldBmp);
        DeleteDC(prevFrameDC);
    }
    if (prevFrameBmp) DeleteObject(prevFrameBmp);

    return static_cast<int>(msg.wParam);
}

// ---- 설정 모드 ----

int ScreenSaverEngine::RunConfigure(HINSTANCE hInst) {
    INITCOMMONCONTROLSEX icc{sizeof(icc), ICC_BAR_CLASSES};
    InitCommonControlsEx(&icc);

    ConfigDialog::Show(hInst, nullptr, desc_, settings_, content_.get());
    return 0;
}

// ---- 미리보기 모드 ----

int ScreenSaverEngine::RunPreview(HINSTANCE hInst, HWND parent) {
    PreviewWindow window;
    if (!window.Init(hInst, parent)) return 1;

    int renderW = window.GetWidth();
    int renderH = window.GetHeight();
    int previewW = (renderW > 0) ? renderW : kDefaultPreviewW;
    int previewH = (renderH > 0) ? renderH : kDefaultPreviewH;

    // 콘텐츠에 미리보기 생성 요청
    auto preview = content_->CreatePreviewContent(previewW, previewH, settings_.forceCPU);
    if (!preview) return 0; // 미리보기 미지원

    window.SetRenderCallback([&](HDC hdc, int w, int h) {
        preview->BlitTo(hdc, w, h);
    });

    // 초기 프레임 렌더
    preview->Update(0.0);
    preview->Render();
    window.RequestRedraw();

    LARGE_INTEGER freq2, prevTime2, currTime2;
    QueryPerformanceFrequency(&freq2);
    QueryPerformanceCounter(&prevTime2);

    // 주기적 WM_TIMER로 GetMessageW 블로킹 해제
    constexpr UINT_PTR kPreviewTimerId = 1;
    SetTimer(window.GetHwnd(), kPreviewTimerId, kPreviewRenderMs, nullptr);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);

        if (msg.message == WM_TIMER && msg.wParam == kPreviewTimerId) {
            QueryPerformanceCounter(&currTime2);
            double dt = static_cast<double>(currTime2.QuadPart - prevTime2.QuadPart) / freq2.QuadPart;
            prevTime2 = currTime2;
            if (dt > kMaxDtPreview) dt = kMaxDtPreview;

            preview->Update(dt);
            preview->Render();
            window.RequestRedraw();
        }
    }

    KillTimer(window.GetHwnd(), kPreviewTimerId);
    return static_cast<int>(msg.wParam);
}

} // namespace wsse
