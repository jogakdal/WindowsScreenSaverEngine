#pragma once
#include <windows.h>

namespace wsse {

enum class Lang { EN, KO, JA, ZH, DE, FR, ES, COUNT };

// 프레임워크 번역 문자열 ID
//
// 프레임워크 공통 문자열만 포함합니다.
// 콘텐츠 고유 문자열은 콘텐츠가 자체 열거형과 번역 테이블로 관리합니다.
enum class Str {
    // ---- 설치/제거 ----
    AdminRequired,
    InstallFailed,       // %lu (error code)
    InstallSuccess,      // %s (app title)
    UninstallConfirm,    // %s (app title)
    UninstallDelayed,    // %s (app title)
    UninstallComplete,   // %s (app title)
    CrossEdReplace,      // %s %s %s (other title, ver, this title)
    CrossEdReplaceBtn,   // %s %s (other title, this title)
    VerReinstall,        // %s (ver)
    ReinstallBtn,
    VerUpgrade,          // %s (ver)
    UpgradeBtn,
    VerDowngrade,        // %s (ver)
    DowngradeBtn,
    ChooseAction,
    InstallBtn,
    PreviewBtn,
    UninstallBtn,

    // ---- 설정 다이얼로그 (프레임워크 공통 컨트롤) ----
    DlgTitleFmt,         // %s (app display name) -> "%s Settings"
    GrpRendering,
    ChkForceCPU,
    GrpDisplay,
    ChkOverlay,
    ChkClock,
    ChkContent,
    GrpUpdate,
    BtnCheckUpdate,
    BtnOK,
    BtnCancel,

    // ---- 업데이트 ----
    Checking,
    NewVersion,          // %s (ver)
    BtnDownload,
    UpToDate,
    CheckFailed,

    // ---- 오버레이 ----
    PressF1,
    CpuInUse,            // %.0f (CPU 사용률 %)
    GpuInUse,            // %d (GPU 부하 %)

    // ---- 시스템 정보 폴백 ----
    UnknownCPU,
    UnknownGPU,

    // ---- 제작자/후원 ----
    AuthorInfo,          // SysLink: blog links
    SponsorMsg,          // plain text

    COUNT
};

class Localization {
public:
    // 초기화: langOverride가 있으면 강제, 없으면 OS 로케일 자동 감지
    // langOverride: "en","ko","ja","zh","de","fr","es" 또는 nullptr
    static void Init(const wchar_t* langOverride = nullptr);
    static const wchar_t* Get(Str id);
    static Lang GetLang() { return lang_; }
    static int GetLangIndex() { return static_cast<int>(lang_); }

private:
    static Lang lang_;
    static Lang DetectLang();
    static Lang ParseCode(const wchar_t* code);
};

// 단축 접근자
inline const wchar_t* TR(Str id) { return Localization::Get(id); }

} // namespace wsse
