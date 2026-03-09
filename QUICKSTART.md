# WSSE 퀵스타트 가이드

이 문서는 WindowsScreenSaverEngine(wsse)을 사용하여 새로운 Windows 스크린세이버를 처음부터 만드는 과정을 단계별로 안내합니다.

## 목차

1. [프로젝트 구조 만들기](#1-프로젝트-구조-만들기)
2. [콘텐츠 클래스 구현](#2-콘텐츠-클래스-구현)
3. [AppDescriptor 설정](#3-appdescriptor-설정)
4. [엔트리 포인트 작성](#4-엔트리-포인트-작성)
5. [리소스 파일 작성](#5-리소스-파일-작성)
6. [빌드 스크립트 작성](#6-빌드-스크립트-작성)
7. [빌드 및 실행](#7-빌드-및-실행)
8. [선택적 기능 확장](#8-선택적-기능-확장)

---

## 1. 프로젝트 구조 만들기

```
MyScreenSaver/
  src/
    MyContent.h
    MyContent.cpp
    main.cpp
  res/
    resource.h
    resource.rc
    config.dlg        (다이얼로그 리소스)
    app.ico           (아이콘, 선택)
  build.bat
```

wsse 라이브러리는 별도 위치에 빌드되어 있다고 가정합니다:
- 헤더: `WindowsScreenSaverEngine/src/` (include 루트)
- 라이브러리: `WindowsScreenSaverEngine/lib/Release/wsse.lib`

---

## 2. 콘텐츠 클래스 구현

### 최소 구현 (MyContent.h)

`IScreenSaverContent`의 순수 가상 함수만 구현하면 동작합니다:

```cpp
#pragma once
#include "framework/IScreenSaverContent.h"
#include "render/RenderSurface.h"

class MyContent : public wsse::IScreenSaverContent {
public:
    // ---- 초기화 ----
    void Init(int renderW, int renderH, bool forceCPU) override;

    // ---- 렌더링 ----
    void BeginRender() override;
    bool IsRenderComplete() const override;
    void FinalizeRender() override;
    bool IsRendering() const override;

    // ---- 출력 ----
    HDC GetSurfaceDC() const override;
    const wsse::RenderSurface& GetSurface() const override;
    void BlitTo(HDC hdc, int destW, int destH) override;

    // ---- 애니메이션 ----
    void Update(double dt) override;
    float GetFadeAlpha() const override;
    bool IsUsingGPU() const override;

private:
    wsse::RenderSurface surface_;
    bool rendering_ = false;

    // 앱 고유 상태
    double time_ = 0.0;
};
```

### 최소 구현 (MyContent.cpp)

```cpp
#include "MyContent.h"
#include <cmath>

void MyContent::Init(int renderW, int renderH, bool forceCPU) {
    (void)forceCPU;
    surface_.Create(renderW, renderH);
}

void MyContent::BeginRender() {
    rendering_ = true;

    // --- 여기에 렌더링 로직 작성 ---
    int w = surface_.GetWidth();
    int h = surface_.GetHeight();
    uint32_t* pixels = surface_.GetPixels();

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            // 예: 시간에 따라 변하는 그라데이션
            double fx = static_cast<double>(x) / w;
            double fy = static_cast<double>(y) / h;
            uint8_t r = static_cast<uint8_t>(127.5 * (1.0 + sin(fx * 6.28 + time_)));
            uint8_t g = static_cast<uint8_t>(127.5 * (1.0 + sin(fy * 6.28 + time_ * 0.7)));
            uint8_t b = static_cast<uint8_t>(127.5 * (1.0 + sin((fx + fy) * 3.14 + time_ * 1.3)));
            pixels[y * w + x] = (0xFF << 24) | (r << 16) | (g << 8) | b;
        }
    }

    rendering_ = false;
}

bool MyContent::IsRenderComplete() const { return !rendering_; }
void MyContent::FinalizeRender() { /* 동기 렌더링이므로 추가 작업 없음 */ }
bool MyContent::IsRendering() const { return rendering_; }

HDC MyContent::GetSurfaceDC() const { return surface_.GetMemDC(); }
const wsse::RenderSurface& MyContent::GetSurface() const { return surface_; }

void MyContent::BlitTo(HDC hdc, int destW, int destH) {
    surface_.StretchBlitTo(hdc, 0, 0, destW, destH);
}

void MyContent::Update(double dt) {
    time_ += dt;
}

float MyContent::GetFadeAlpha() const { return 1.0f; }
bool MyContent::IsUsingGPU() const { return false; }
```

> **핵심 포인트**: `BeginRender()`에서 `surface_`의 픽셀 버퍼에 직접 그리면 됩니다. 프레임워크가 타이밍, 화면 출력, 오버레이 합성을 모두 처리합니다.

---

## 3. AppDescriptor 설정

`AppDescriptor`는 프레임워크에 앱의 메타데이터와 활성화할 기능을 알려줍니다.

```cpp
wsse::AppDescriptor GetDescriptor() {
    wsse::AppDescriptor desc{};

    // 필수: 파일명과 표시 이름
    desc.scrName = L"MyScreenSaver.scr";
    desc.appTitle = L"My Screen Saver";

    // 필수: 레지스트리 키 (설정 저장 경로)
    desc.registryKey = L"Software\\MyScreenSaver";

    // 설정 다이얼로그 리소스 ID
    desc.dialogResourceId = IDD_CONFIG;

    // 기능 플래그 (기본값: 모두 true)
    desc.hasOverlay = true;     // 시스템 정보 오버레이
    desc.hasClock = true;       // 7-세그먼트 시계
    desc.hasAutoUpdate = false; // 자동 업데이트 (GitHub 미사용 시 false)

    return desc;
}
```

### 자동 업데이트 활성화 시

GitHub Releases를 사용한다면:

```cpp
desc.hasAutoUpdate = true;
desc.updateHost = L"api.github.com";
desc.updatePath = L"/repos/사용자명/저장소명/releases/latest";
desc.userAgent = L"MyScreenSaver/1.0";
```

---

## 4. 엔트리 포인트 작성

`main.cpp`는 단 몇 줄입니다:

```cpp
#include <windows.h>
#include "framework/ScreenSaverEngine.h"
#include "MyContent.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) {
    auto desc = GetDescriptor();   // 3단계에서 정의한 함수
    auto content = std::make_unique<MyContent>();

    wsse::ScreenSaverEngine engine(desc, std::move(content));
    return engine.Run(hInstance);
}
```

`ScreenSaverEngine::Run()`이 커맨드라인을 자동 파싱하여 모드별로 분기합니다:
- `/s` -> 전체화면 스크린세이버
- `/c` -> 설정 다이얼로그
- `/p <hwnd>` -> 제어판 미리보기
- `/i` -> 설치, `/u` -> 제거
- 인자 없음 -> 런처 (설치/미리보기/제거 선택)

---

## 5. 리소스 파일 작성

### resource.h

프레임워크 표준 컨트롤 ID와 앱 고유 ID를 함께 정의합니다.
프레임워크 ID는 `ControlIds.h`에 정의된 값과 일치해야 합니다:

```c
#pragma once

// 다이얼로그
#define IDD_CONFIG          101

// 프레임워크 표준 컨트롤 (ControlIds.h의 값과 동일)
// 다이얼로그에 포함하면 프레임워크가 자동 관리, 생략하면 건너뜀
#define IDC_GRP_RENDERING   1020
#define IDC_FORCE_CPU       1006
#define IDC_GRP_DISPLAY     1022
#define IDC_SHOW_OVERLAY    1008
#define IDC_SHOW_CLOCK      1009
#define IDC_GRP_UPDATE      1023
#define IDC_UPDATE_CHECK    1010
#define IDC_UPDATE_STATUS   1011
#define IDC_VERSION_LABEL   1012

// 앱 고유 컨트롤 (1100번대 이상 권장)
#define IDC_MY_SLIDER       1100
#define IDC_MY_LABEL        1101

// desk.cpl 열거용 설명 문자열
#define IDS_DESCRIPTION     1
```

### resource.rc

```c
#include <windows.h>
#include "resource.h"

// desk.cpl이 스크린세이버를 인식하려면 필수
STRINGTABLE
BEGIN
    IDS_DESCRIPTION  "My Screen Saver"
END

// 설정 다이얼로그
IDD_CONFIG DIALOGEX 0, 0, 260, 200
STYLE DS_MODALFRAME | DS_CENTER | WS_POPUP | WS_CAPTION | WS_SYSMENU
CAPTION "Settings"
FONT 9, "Segoe UI"
BEGIN
    // 렌더링 그룹
    GROUPBOX    "Rendering", IDC_GRP_RENDERING, 8, 8, 244, 36
    AUTOCHECKBOX "Force CPU rendering", IDC_FORCE_CPU, 16, 22, 120, 10

    // 앱 고유 컨트롤
    // LTEXT       "My Setting:", IDC_MY_LABEL, 16, 56, 60, 10
    // CONTROL     "", IDC_MY_SLIDER, "msctls_trackbar32", TBS_BOTH, 80, 54, 160, 16

    // 디스플레이 그룹
    GROUPBOX    "Display", IDC_GRP_DISPLAY, 8, 80, 244, 40
    AUTOCHECKBOX "Show system info", IDC_SHOW_OVERLAY, 16, 94, 120, 10
    AUTOCHECKBOX "Show clock", IDC_SHOW_CLOCK, 16, 106, 120, 10

    // 버전 표시
    LTEXT       "", IDC_VERSION_LABEL, 8, 180, 120, 10

    // 확인/취소
    DEFPUSHBUTTON "OK", IDOK, 148, 178, 50, 14
    PUSHBUTTON  "Cancel", IDCANCEL, 202, 178, 50, 14
END
```

> **참고**: 다이얼로그의 텍스트는 영어로 작성해도 됩니다. 프레임워크가 `Localization`을 통해 표준 컨트롤의 텍스트를 OS 언어에 맞게 자동 교체합니다.

---

## 6. 빌드 스크립트 작성

```bat
@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

set VCDIR=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207
set WINSDK=C:\Program Files (x86)\Windows Kits\10
set WINSDKVER=10.0.26100.0

set PATH=%VCDIR%\bin\Hostx64\x64;%WINSDK%\bin\%WINSDKVER%\x64;%PATH%
set INCLUDE=%VCDIR%\include;%WINSDK%\Include\%WINSDKVER%\ucrt;%WINSDK%\Include\%WINSDKVER%\um;%WINSDK%\Include\%WINSDKVER%\shared
set LIB=%VCDIR%\lib\x64;%WINSDK%\Lib\%WINSDKVER%\ucrt\x64;%WINSDK%\Lib\%WINSDKVER%\um\x64

rem --- 경로 설정 ---
set SRCDIR=%~dp0src
set RESDIR=%~dp0res
set OUTDIR=%~dp0bin\Release
set OBJDIR=%~dp0obj\Release
set WSSE_INC=C:\Users\jogak\project\ScreenSaver\WindowsScreenSaverEngine\src
set WSSE_LIB=C:\Users\jogak\project\ScreenSaver\WindowsScreenSaverEngine\lib\Release\wsse.lib

if not exist "%OUTDIR%" mkdir "%OUTDIR%"
if not exist "%OBJDIR%" mkdir "%OBJDIR%"

set CFLAGS=/nologo /c /O2 /GL /MT /W4 /EHsc /std:c++20 /utf-8 /fp:fast ^
    /DUNICODE /D_UNICODE /DNOMINMAX /I"%SRCDIR%" /I"%WSSE_INC%"

echo [1/3] Compiling...
for %%f in (main MyContent) do (
    echo   %%f.cpp
    cl %CFLAGS% /Fo"%OBJDIR%\%%f.obj" "%SRCDIR%\%%f.cpp"
    if errorlevel 1 goto :error
)

echo [2/3] Compiling resources...
rc /nologo /fo"%OBJDIR%\resource.res" "%RESDIR%\resource.rc"
if errorlevel 1 goto :error

echo [3/3] Linking...
link /nologo /LTCG /OUT:"%OUTDIR%\MyScreenSaver.scr" /SUBSYSTEM:WINDOWS ^
    "%OBJDIR%\main.obj" "%OBJDIR%\MyContent.obj" "%OBJDIR%\resource.res" ^
    "%WSSE_LIB%" ^
    user32.lib gdi32.lib gdiplus.lib shell32.lib advapi32.lib ^
    comctl32.lib winhttp.lib msimg32.lib comdlg32.lib
if errorlevel 1 goto :error

echo Build successful!
echo Output: %OUTDIR%\MyScreenSaver.scr
goto :end

:error
echo BUILD FAILED!
exit /b 1

:end
echo Done.
```

### 링크에 필요한 시스템 라이브러리

| 라이브러리 | 용도 |
|-----------|------|
| `user32.lib` | 윈도우, 메시지, 다이얼로그 |
| `gdi32.lib` | GDI 렌더링, DIB 섹션 |
| `gdiplus.lib` | PNG 저장 |
| `shell32.lib` | 파일 복사 (설치) |
| `advapi32.lib` | 레지스트리 |
| `comctl32.lib` | 공용 컨트롤 (TaskDialog) |
| `winhttp.lib` | 자동 업데이트 HTTP |
| `msimg32.lib` | AlphaBlend (시계 오버레이) |
| `comdlg32.lib` | 공용 다이얼로그 |

> `hasAutoUpdate = false`이면 `winhttp.lib`는 생략 가능합니다. 단, wsse.lib가 심볼을 포함하고 있어 링커가 요구할 수 있으므로 포함을 권장합니다.

---

## 7. 빌드 및 실행

```bat
rem wsse 라이브러리 빌드 (최초 1회)
cmd.exe /c "C:\...\WindowsScreenSaverEngine\build.bat"

rem 앱 빌드
cmd.exe /c build.bat

rem 전체화면 실행
bin\Release\MyScreenSaver.scr /s

rem 설정 다이얼로그
bin\Release\MyScreenSaver.scr /c

rem System32에 설치
bin\Release\MyScreenSaver.scr /i
```

---

## 8. 선택적 기능 확장

필수 구현만으로도 동작하지만, 선택적 메서드를 오버라이드하여 기능을 확장할 수 있습니다.

### 설정 저장/로드

레지스트리를 통해 앱 고유 설정을 저장합니다:

```cpp
#include "framework/Registry.h"

void MyContent::LoadSettings() {
    myValue_ = wsse::Registry::ReadInt(L"MyValue", 50);
}

void MyContent::SaveSettings() {
    wsse::Registry::WriteInt(L"MyValue", myValue_);
}
```

### 설정 다이얼로그 연동

다이얼로그의 앱 고유 컨트롤을 초기화/읽기합니다:

```cpp
void MyContent::InitConfigControls(HWND dlg) {
    // 슬라이더 초기화
    SendDlgItemMessage(dlg, IDC_MY_SLIDER, TBM_SETRANGE, TRUE, MAKELPARAM(1, 100));
    SendDlgItemMessage(dlg, IDC_MY_SLIDER, TBM_SETPOS, TRUE, myValue_);
}

void MyContent::ReadConfigControls(HWND dlg) {
    myValue_ = (int)SendDlgItemMessage(dlg, IDC_MY_SLIDER, TBM_GETPOS, 0, 0);
    SaveSettings();
}
```

### 오버레이 동적 라인

시스템 정보 아래에 앱 고유 정보를 표시합니다:

```cpp
void MyContent::FormatOverlayLine(wchar_t* buf, int bufSize,
                                   double fps, double cpuUsage) {
    swprintf_s(buf, bufSize,
               L"FPS: %.0f | CPU: %.0f%% | Frame: %d",
               fps, cpuUsage, frameCount_);
}
```

### 비동기 렌더링

렌더링이 오래 걸리는 경우 별도 스레드에서 실행합니다:

```cpp
void MyContent::BeginRender() {
    rendering_ = true;
    renderThread_ = std::thread([this]() {
        // 무거운 렌더링 작업
        HeavyRender();
        rendering_ = false;
    });
    renderThread_.detach();
}

bool MyContent::IsRenderComplete() const { return !rendering_; }
```

프레임워크는 `IsRenderComplete()`가 `false`인 동안 이전 프레임을 계속 표시합니다. `SupportsInterpolation()`을 `true`로 오버라이드하면, 렌더링 대기 중 이전 프레임을 확대/축소하여 부드러운 전환 효과를 제공할 수 있습니다.

### 미리보기 콘텐츠

제어판의 작은 미리보기 창에 표시할 경량 콘텐츠:

```cpp
std::unique_ptr<wsse::IPreviewContent> MyContent::CreatePreviewContent(
        int width, int height, bool forceCPU) {
    auto preview = std::make_unique<MyPreviewContent>();
    preview->Init(width, height, forceCPU);
    return preview;
}
```

`IPreviewContent`는 4개의 순수 가상 함수만 구현하면 됩니다:
- `Init()` - 초기화
- `Update(dt)` - 애니메이션 갱신
- `Render()` - 동기 렌더링
- `BlitTo()` - 화면 출력

---

## 프레임워크 동작 흐름

```
ScreenSaverEngine::Run()
  |
  +-- 커맨드라인 파싱 -> AppMode 결정
  |
  +-- /s: RunScreenSaver()
  |     |-- content->LoadSettings()
  |     |-- content->Init(w, h, forceCPU)
  |     |-- 메시지 루프:
  |     |     |-- content->Update(dt)
  |     |     |-- content->BeginRender()
  |     |     |-- content->IsRenderComplete()?
  |     |     |     +-- true: content->FinalizeRender()
  |     |     |     +-- false: 이전 프레임 유지 (보간 가능)
  |     |     |-- content->BlitTo(hdc)
  |     |     |-- [오버레이 합성]
  |     |     +-- [시계 합성]
  |     +-- 마우스/키보드 -> 종료
  |
  +-- /c: RunConfigure()
  |     +-- ConfigDialog::Show()
  |           |-- content->InitConfigControls()
  |           |-- [사용자 조작]
  |           +-- content->ReadConfigControls()
  |
  +-- /p: RunPreview()
  |     +-- content->CreatePreviewContent()
  |           |-- preview->Init()
  |           |-- preview->Update(dt)
  |           |-- preview->Render()
  |           +-- preview->BlitTo()
  |
  +-- /i: DoInstall()    -- System32에 .scr 복사
  +-- /u: DoUninstall()  -- System32에서 .scr 삭제
  +-- (없음): ShowLauncher()
```
