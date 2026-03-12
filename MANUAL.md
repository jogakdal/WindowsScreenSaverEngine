# WSSE 프레임워크 매뉴얼

WindowsScreenSaverEngine(wsse) 프레임워크의 아키텍처, API 레퍼런스, 확장 가이드를 다룹니다.

처음 사용한다면 [QUICKSTART.md](QUICKSTART.md)를 먼저 읽으세요.

---

## 목차

1. [아키텍처 개요](#1-아키텍처-개요)
2. [라이프사이클](#2-라이프사이클)
3. [API 레퍼런스: IScreenSaverContent](#3-api-레퍼런스-iscreensavercontent)
4. [API 레퍼런스: IPreviewContent](#4-api-레퍼런스-ipreviewcontent)
5. [API 레퍼런스: AppDescriptor](#5-api-레퍼런스-appdescriptor)
6. [API 레퍼런스: FrameworkSettings](#6-api-레퍼런스-frameworksettings)
7. [API 레퍼런스: ScreenSaverEngine](#7-api-레퍼런스-screensaverengine)
8. [API 레퍼런스: RenderSurface](#8-api-레퍼런스-rendersurface)
9. [API 레퍼런스: Registry](#9-api-레퍼런스-registry)
10. [API 레퍼런스: Localization](#10-api-레퍼런스-localization)
11. [API 레퍼런스: UpdateChecker](#11-api-레퍼런스-updatechecker)
12. [API 레퍼런스: TextOverlay](#12-api-레퍼런스-textoverlay)
13. [API 레퍼런스: ClockOverlay](#13-api-레퍼런스-clockoverlay)
14. [API 레퍼런스: SystemInfo](#14-api-레퍼런스-systeminfo)
15. [API 레퍼런스: SegmentDisplay](#15-api-레퍼런스-segmentdisplay)
16. [API 레퍼런스: Window](#16-api-레퍼런스-window)
17. [API 레퍼런스: ScreenSaverWindow](#17-api-레퍼런스-screensaverwindow)
18. [API 레퍼런스: PreviewWindow](#18-api-레퍼런스-previewwindow)
19. [API 레퍼런스: ConfigDialog](#19-api-레퍼런스-configdialog)
20. [API 레퍼런스: ControlIds](#20-api-레퍼런스-controlids)
21. [설정 다이얼로그 가이드](#21-설정-다이얼로그-가이드)
22. [비동기 렌더링 가이드](#22-비동기-렌더링-가이드)
23. [시간 보간 가이드](#23-시간-보간-가이드)
24. [다중 모니터 가이드](#24-다중-모니터-가이드)
25. [커맨드라인 레퍼런스](#25-커맨드라인-레퍼런스)
26. [링크 의존성](#26-링크-의존성)

---

## 1. 아키텍처 개요

wsse는 콘텐츠 분리 아키텍처를 사용합니다:

```
+-------------------+       +----------------------+
| ScreenSaverEngine |<>---->| IScreenSaverContent  |
| (프레임워크)       |       | (앱이 구현)           |
+-------------------+       +----------------------+
  - 커맨드라인 파싱            - Init / BeginRender
  - 윈도우 관리                - Update / BlitTo
  - 메시지 루프                - 설정 저장/로드
  - 프레임 타이밍              - 오버레이 동적 라인
  - 오버레이/시계 합성          - 미리보기 콘텐츠
  - 시간 보간
  - 다중 모니터 미러링
  - 스크린샷
  - 설치/제거
  - 자동 업데이트
```

**프레임워크가 담당하는 것**: 윈도우 생성, 메시지 루프, 입력 처리(마우스/키보드 종료), 프레임 타이밍, 오버레이/시계 합성, 스크린샷 저장, System32 설치/제거, 관리자 권한 승격, 설정 다이얼로그(공통 컨트롤), 레지스트리 관리, 다국어, 자동 업데이트, 다중 모니터 미러링.

**콘텐츠가 담당하는 것**: 실제 화면에 표시될 그래픽을 렌더링하고 애니메이션하는 것. `RenderSurface`의 픽셀 버퍼에 그리기만 하면 프레임워크가 나머지를 처리합니다.

---

## 2. 라이프사이클

### 전체화면 모드 (/s)

```
ScreenSaverEngine::Run()
  |-- SetProcessDPIAware()
  |-- Localization::Init()
  |-- Registry::SetKeyPath()
  |-- Registry::LoadFrameworkSettings()
  |-- content->LoadSettings()
  |
  +-- RunScreenSaver()
        |-- ScreenSaverWindow::Init()         // 전체화면 창 + 미러 창
        |-- content->GetRenderSize()          // 렌더링 해상도 결정
        |-- content->Init(renderW, renderH, forceCPU)
        |-- SystemInfo::Collect()             // (hasOverlay일 때)
        |-- TextOverlay::Init()               // (hasOverlay일 때)
        |-- ClockOverlay::Init()              // (hasClock && showClock일 때)
        |
        |-- [메시지 루프]
        |     |-- content->Update(dt)
        |     |-- content->BeginRender()
        |     |-- content->IsRenderComplete()?
        |     |     +-- true:
        |     |     |     content->FinalizeRender()
        |     |     |     [시간 보간용 프레임 백업]
        |     |     |     [FPS/CPU 측정]
        |     |     |     [스크린샷 저장]
        |     |     +-- false:
        |     |           [시간 보간 -- 이전 프레임 확대/축소]
        |     |
        |     |-- [렌더 콜백]
        |     |     |-- UpdateMirrorWindows()    // 보조 모니터에 순수 콘텐츠 복사
        |     |     |-- ClockOverlay::Render()   // 이후 오버레이/시계는 프라이머리만
        |     |     |-- content->FormatOverlayLine()
        |     |     |-- TextOverlay::Render()
        |     |     +-- content->BlitTo(hdc)
        |     |
        |     +-- 마우스/키보드 -> WM_QUIT
        |
        +-- 정리
```

### 설정 모드 (/c)

```
ScreenSaverEngine::Run()
  +-- RunConfigure()
        +-- ConfigDialog::Show()
              |-- content->InitConfigControls(dlg)
              |-- [사용자 조작]
              +-- content->ReadConfigControls(dlg)
```

### 미리보기 모드 (/p)

```
ScreenSaverEngine::Run()
  +-- RunPreview()
        |-- PreviewWindow::Init(parent)
        |-- content->CreatePreviewContent(w, h, forceCPU)
        +-- [타이머 루프 (500ms 간격)]
              |-- preview->Update(dt)
              |-- preview->Render()
              +-- preview->BlitTo(hdc)
```

### 설치/제거

```
(인자 없음, .exe) -> ShowLauncher() -- TaskDialog로 설치/미리보기/제거 선택
/i 또는 /install   -> ElevateIfNeeded() -> DoInstall()
/u                 -> ElevateIfNeeded() -> DoUninstall()
```

---

## 3. API 레퍼런스: IScreenSaverContent

헤더: `framework/IScreenSaverContent.h`

스크린세이버의 렌더링, 애니메이션, 설정을 구현하는 핵심 인터페이스. `= 0`으로 표시된 순수 가상 함수는 반드시 구현해야 하며, 나머지는 기본 동작을 제공합니다.

### 초기화

| 메서드 | 설명 |
|--------|------|
| `void Init(int renderW, int renderH, bool forceCPU) = 0` | 렌더러 초기화. `forceCPU`가 `true`이면 GPU 사용 불가. |
| `void GetRenderSize(int screenW, int screenH, int& renderW, int& renderH) const` | 화면 크기에서 실제 렌더링 크기 결정. 기본: 화면 크기 그대로 사용. 축소 렌더링 시 오버라이드. |
| `bool IsLite() const` | Lite(경량) 버전 여부. 오버레이에 해상도 정보 표시에 사용. 기본: `false`. |

### 비동기 렌더링

| 메서드 | 설명 |
|--------|------|
| `void BeginRender() = 0` | 새 프레임 렌더링 시작. 비동기 구현이면 즉시 반환. |
| `bool IsRenderComplete() const = 0` | 현재 렌더링이 완료되었는지 확인. |
| `void FinalizeRender() = 0` | 완료된 렌더링 결과를 표면에 반영. `IsRenderComplete() == true`일 때만 호출됨. |
| `bool IsRendering() const = 0` | 렌더링 진행 중 여부. |

### 출력

| 메서드 | 설명 |
|--------|------|
| `HDC GetSurfaceDC() const = 0` | 렌더 표면의 메모리 DC. 오버레이 합성에 사용. |
| `const RenderSurface& GetSurface() const = 0` | 렌더 표면 참조. 스크린샷 저장에 사용. |
| `void BlitTo(HDC hdc, int destW, int destH) = 0` | 렌더 결과를 대상 DC에 복사. `destW/destH`는 화면 크기(렌더 크기와 다를 수 있음). |

### 애니메이션

| 메서드 | 설명 |
|--------|------|
| `void Update(double dt) = 0` | 매 프레임 애니메이션 상태 갱신. `dt`는 경과 시간(초). |
| `float GetFadeAlpha() const = 0` | 현재 페이드 알파 (0.0 ~ 1.0). 오버레이/시계 밝기 조절에 사용. |
| `bool IsFadingOut() const` | 페이드아웃 진행 중 여부. 렌더 스케줄링 최적화에 사용. 기본: `false`. |
| `bool IsUsingGPU() const = 0` | GPU 렌더링 사용 여부. `false`이면 CPU 프레임 캡(30fps) 적용. |
| `int GetCycleCount() const` | 스타일/애니메이션 순환 완료 횟수. 스크린샷 모드에서 1회 완료 시 종료. 기본: `0`. |

### 페이드 최적화 (선택)

| 메서드 | 설명 |
|--------|------|
| `bool CanReapplyFade() const` | 기존 렌더 결과에 페이드 알파만 재적용 가능 여부. 기본: `false`. |
| `void ReapplyFade(float alpha)` | 기존 렌더 결과에 지정 알파 재적용. 재계산 없이 페이드 전환 처리. |

### 시간 보간 (선택)

비동기 렌더링 중 이전 프레임을 확대/축소하여 부드러운 전환을 제공하는 기능.

| 메서드 | 설명 |
|--------|------|
| `bool SupportsInterpolation() const` | 시간 보간 지원 여부. 기본: `false`. |
| `InterpInfo GetInterpInfo() const` | 보간에 필요한 스케일/중심 정보 반환. |

**InterpInfo 구조체**:

| 필드 | 설명 |
|------|------|
| `double prevScale` | 이전 렌더 프레임의 스케일 |
| `double currScale` | 현재 애니메이션 스케일 |
| `double prevCenterX/Y` | 이전 렌더 프레임의 중심 좌표 |
| `double currCenterX/Y` | 현재 애니메이션 중심 좌표 |
| `float aspectRatio` | 가로/세로 비율 (기본: 1.0) |

프레임워크는 `prevScale / currScale`의 비율을 계산하여 이전 프레임을 `StretchBlt`로 확대합니다. 줌인 애니메이션(프랙탈 등)에서 렌더링 대기 중 부드러운 전환을 제공합니다.

### 설정

| 메서드 | 설명 |
|--------|------|
| `void LoadSettings()` | 레지스트리에서 콘텐츠 고유 설정 로드. `Registry::ReadInt` 등 사용. |
| `void SaveSettings()` | 레지스트리에 콘텐츠 고유 설정 저장. |
| `bool ApplyFrameworkSettings(int renderW, int renderH, bool forceCPU)` | 프레임워크 설정 변경 적용. `true` 반환 시 렌더 파이프라인 재초기화. |

### 설정 다이얼로그

| 메서드 | 설명 |
|--------|------|
| `void InitConfigControls(HWND dlg)` | 다이얼로그 열릴 때 콘텐츠 고유 컨트롤 초기화. |
| `void ReadConfigControls(HWND dlg)` | OK 클릭 시 콘텐츠 고유 컨트롤에서 값 읽기 및 저장. |
| `bool HandleConfigMessage(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)` | 다이얼로그 메시지 처리. `true` 반환 시 프레임워크가 추가 처리하지 않음. |

### 오버레이

| 메서드 | 설명 |
|--------|------|
| `void FormatOverlayLine(wchar_t* buf, int bufSize, double fps)` | 동적 정보 라인 포맷. 시스템 정보 아래 표시. CPU 사용률은 프레임워크가 CPU 라인에 표시. `buf`를 비워두면 해당 줄 생략. |

### 미리보기

| 메서드 | 설명 |
|--------|------|
| `unique_ptr<IPreviewContent> CreatePreviewContent(int w, int h, bool forceCPU)` | 제어판 미리보기용 경량 콘텐츠 생성. `nullptr` 반환 시 미리보기 비활성. |

---

## 4. API 레퍼런스: IPreviewContent

헤더: `framework/IPreviewContent.h`

제어판 화면 보호기 미리보기 창(`/p` 모드)에 표시할 콘텐츠 인터페이스. 메인 콘텐츠와 별도로 구현하여 경량 렌더링을 제공합니다. 모든 메서드가 순수 가상입니다.

| 메서드 | 설명 |
|--------|------|
| `void Init(int width, int height, bool forceCPU) = 0` | 렌더러 초기화. 미리보기 창 크기는 보통 152x112. |
| `void Update(double dt) = 0` | 애니메이션 상태 갱신. |
| `void Render() = 0` | 한 프레임 동기 렌더링 (완료까지 블록). |
| `void BlitTo(HDC hdc, int destW, int destH) = 0` | 렌더링 결과를 대상 DC에 복사. |

---

## 5. API 레퍼런스: AppDescriptor

헤더: `framework/AppDescriptor.h`

앱 메타데이터와 기능 플래그를 정의하는 구조체. `ScreenSaverEngine` 생성 시 전달합니다.

### 파일 및 표시 이름

| 필드 | 타입 | 설명 |
|------|------|------|
| `scrName` | `const wchar_t*` | System32에 설치되는 .scr 파일명. 예: `L"MyApp.scr"` |
| `appTitle` | `const wchar_t*` | 메시지 박스 제목 등에 쓰이는 짧은 앱 이름. 예: `L"MyApp"` |
| `otherScrName` | `const wchar_t*` | Full/Lite 교차 에디션 파일명. 없으면 `nullptr`. |
| `otherAppTitle` | `const wchar_t*` | 교차 에디션 표시 이름. 없으면 `nullptr`. |

### 레지스트리

| 필드 | 타입 | 설명 |
|------|------|------|
| `registryKey` | `const wchar_t*` | HKCU 하위 설정 저장 키. 예: `L"Software\\MyApp"` |

### 자동 업데이트

| 필드 | 타입 | 설명 |
|------|------|------|
| `updateHost` | `const wchar_t*` | API 호스트. 예: `L"api.github.com"` |
| `updatePath` | `const wchar_t*` | API 경로. 예: `L"/repos/user/repo/releases/latest"` |
| `userAgent` | `const wchar_t*` | HTTP User-Agent. 예: `L"MyApp/1.0"` |

### 기능 플래그

| 필드 | 타입 | 기본값 | 설명 |
|------|------|--------|------|
| `hasOverlay` | `bool` | `true` | 텍스트 오버레이 (시스템 정보 + 동적 라인) |
| `hasClock` | `bool` | `true` | 7-세그먼트 디지털 시계 |
| `hasAutoUpdate` | `bool` | `true` | GitHub Releases 자동 업데이트 |

`false`로 설정하면 해당 기능이 엔진에서 초기화/렌더링되지 않고, 설정 다이얼로그에서도 관련 컨트롤이 숨겨집니다.

### 리소스

| 필드 | 타입 | 설명 |
|------|------|------|
| `dialogResourceId` | `int` | 설정 다이얼로그 리소스 ID. 예: `IDD_CONFIG` (101) |

---

## 6. API 레퍼런스: FrameworkSettings

헤더: `framework/FrameworkSettings.h`

모든 스크린세이버에서 공유하는 프레임워크 공통 설정.

| 필드 | 타입 | 기본값 | 설명 |
|------|------|--------|------|
| `forceCPU` | `bool` | `false` | GPU 대신 CPU 렌더링 강제 |
| `showOverlay` | `bool` | `true` | 시스템 정보 오버레이 표시 |
| `showClock` | `bool` | `true` | 디지털 시계 표시 |

`Registry::LoadFrameworkSettings()`로 레지스트리에서 로드하고, `Registry::SaveFrameworkSettings()`로 저장합니다. `ConfigDialog`가 이 과정을 자동으로 처리합니다.

---

## 7. API 레퍼런스: ScreenSaverEngine

헤더: `framework/ScreenSaverEngine.h`

스크린세이버의 전체 라이프사이클을 관리하는 엔진 클래스.

### 생성

```cpp
ScreenSaverEngine(const AppDescriptor& desc,
                  std::unique_ptr<IScreenSaverContent> content);
```

`desc`와 `content`를 받아 엔진을 초기화합니다. `content`의 소유권이 엔진으로 이동합니다.

### 실행

```cpp
int Run(HINSTANCE hInst);
```

커맨드라인을 자동 파싱하여 모드별로 분기합니다. 반환값은 프로세스 종료 코드로 사용합니다.

`Run()`이 내부에서 수행하는 초기화 순서:

1. `SetProcessDPIAware()` -- DPI 가상화 비활성화
2. `Localization::Init()` -- `/lang:xx` 인자 또는 OS 로케일 기반
3. `Registry::SetKeyPath()` -- `desc.registryKey` 기반
4. `Registry::LoadFrameworkSettings()` -- 프레임워크 설정 로드
5. `content->LoadSettings()` -- 콘텐츠 고유 설정 로드
6. 모드 분기 (`/s`, `/c`, `/p`, `/i`, `/u`, 런처)

### AppMode 열거형

```cpp
enum class AppMode {
    ScreenSaver,  // /s - 전체화면
    Configure,    // /c - 설정 다이얼로그
    Preview       // /p <hwnd> - 미리보기
};
```

### ScreenshotConfig 구조체

```cpp
struct ScreenshotConfig {
    int intervalSec = 0;              // 0 = 비활성
    wchar_t folder[MAX_PATH] = {};    // 저장 폴더
};
```

`/screenshot:N[:폴더]` 인자로 자동 설정됩니다.

### 프레임 타이밍 상수

| 상수 | 값 | 설명 |
|------|------|------|
| `kMaxDtScreenSaver` | 0.1 | 스크린세이버 모드 최대 dt (초) |
| `kMaxDtPreview` | 1.0 | 미리보기 모드 최대 dt (초) |
| `kCPUTargetFrameTime` | 1/30 | CPU 렌더링 시 목표 프레임 시간 (~30fps) |
| `kInterpFrameTime` | 1/60 | 시간 보간 프레임 시간 (~60fps) |
| `kPreviewRenderMs` | 500 | 미리보기 타이머 간격 (ms) |

---

## 8. API 레퍼런스: RenderSurface

헤더: `render/RenderSurface.h`

Win32 DIB 섹션을 래핑한 픽셀 버퍼. 콘텐츠가 렌더링 결과를 저장하는 주요 표면입니다.

### 생성/해제

| 메서드 | 설명 |
|--------|------|
| `bool Create(int width, int height)` | DIB 섹션 생성. 32bpp BGRA, top-down. 성공 시 `true`. |
| `~RenderSurface()` | DIB 섹션 및 DC 해제. |

### 픽셀 접근

| 메서드 | 설명 |
|--------|------|
| `uint32_t* GetPixels()` | 픽셀 버퍼 포인터 (BGRA 순서). |
| `const uint32_t* GetPixels() const` | const 버전. |
| `int GetWidth() const` | 폭 (px). |
| `int GetHeight() const` | 높이 (px). |
| `int GetStride() const` | uint32_t 단위 스트라이드 (= width). |

### 픽셀 쓰기

| 메서드 | 설명 |
|--------|------|
| `void SetPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b)` | RGB로 한 픽셀 설정. |
| `void SetPixelBGRA(int x, int y, uint32_t bgra)` | BGRA 32비트 값으로 한 픽셀 설정. |
| `void Clear(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0)` | 전체 버퍼를 지정 색상으로 클리어. |

### 화면 출력

| 메서드 | 설명 |
|--------|------|
| `void BlitTo(HDC destDC, int destX, int destY) const` | 1:1 복사 (BitBlt). |
| `void StretchBlitTo(HDC destDC, int destX, int destY, int destW, int destH) const` | 스케일 복사 (StretchBlt). |
| `void DirectBlitTo(HDC destDC) const` | GDI DPI 가상화 우회 직접 픽셀 복사. |

### DC 접근

| 메서드 | 설명 |
|--------|------|
| `HDC GetMemDC() const` | DIB 섹션의 메모리 DC. 오버레이 합성 등에 사용. |

### 파일 저장

| 메서드 | 설명 |
|--------|------|
| `bool SaveBMP(const wchar_t* path) const` | BMP 파일로 저장. |
| `bool SavePNG(const wchar_t* path) const` | PNG 파일로 저장 (GDI+). |

### 픽셀 포맷

- 32bpp BGRA (Blue-Green-Red-Alpha), top-down 배열
- `pixels[y * width + x]` = `0xAARRGGBB` (메모리 상 BGRA 순서, uint32_t 값은 ARGB)
- 직접 쓸 때: `pixels[y * w + x] = (0xFF << 24) | (r << 16) | (g << 8) | b;`

---

## 9. API 레퍼런스: Registry

헤더: `framework/Registry.h`

HKCU 레지스트리 설정 관리 유틸리티. 모든 메서드가 static입니다.

### 키 경로 설정

| 메서드 | 설명 |
|--------|------|
| `static void SetKeyPath(const wchar_t* path)` | 레지스트리 키 경로 설정 (앱 시작 시 한 번). 예: `"Software\\MyApp"` |
| `static const wchar_t* GetKeyPath()` | 현재 키 경로 반환. |

`ScreenSaverEngine::Run()`이 `AppDescriptor::registryKey`로 자동 호출합니다.

### 프레임워크 설정

| 메서드 | 설명 |
|--------|------|
| `static void LoadFrameworkSettings(FrameworkSettings& settings)` | 프레임워크 공통 설정 로드. |
| `static void SaveFrameworkSettings(const FrameworkSettings& settings)` | 프레임워크 공통 설정 저장. |

### 범용 접근 (콘텐츠용)

| 메서드 | 설명 |
|--------|------|
| `static bool ReadBool(const wchar_t* name, bool defaultVal)` | bool 값 읽기. 없으면 기본값. |
| `static int ReadInt(const wchar_t* name, int defaultVal)` | int 값 읽기. 없으면 기본값. |
| `static void WriteBool(const wchar_t* name, bool val)` | bool 값 쓰기. |
| `static void WriteInt(const wchar_t* name, int val)` | int 값 쓰기. |

사용 예:

```cpp
void MyContent::LoadSettings() {
    speed_ = wsse::Registry::ReadInt(L"Speed", 50);
    colorful_ = wsse::Registry::ReadBool(L"Colorful", true);
}

void MyContent::SaveSettings() {
    wsse::Registry::WriteInt(L"Speed", speed_);
    wsse::Registry::WriteBool(L"Colorful", colorful_);
}
```

---

## 10. API 레퍼런스: Localization

헤더: `framework/Localization.h`

프레임워크 공통 문자열의 다국어 번역. 7개 언어 지원: EN, KO, JA, ZH, DE, FR, ES.

### Localization 클래스

| 메서드 | 설명 |
|--------|------|
| `static void Init(const wchar_t* langOverride = nullptr)` | 초기화. `langOverride`가 있으면 강제, 없으면 OS 로케일 자동 감지. |
| `static const wchar_t* Get(Str id)` | 문자열 ID로 현재 언어의 번역 문자열 반환. |
| `static Lang GetLang()` | 현재 언어 반환. |
| `static int GetLangIndex()` | 현재 언어의 정수 인덱스 반환. |

### 단축 접근자

```cpp
inline const wchar_t* TR(Str id) { return Localization::Get(id); }
```

### Lang 열거형

```cpp
enum class Lang { EN, KO, JA, ZH, DE, FR, ES, COUNT };
```

### Str 열거형 (프레임워크 번역 문자열 ID)

**설치/제거**: `AdminRequired`, `InstallFailed`, `InstallSuccess`, `UninstallConfirm`, `UninstallDelayed`, `UninstallComplete`, `CrossEdReplace`, `CrossEdReplaceBtn`, `VerReinstall`, `ReinstallBtn`, `VerUpgrade`, `UpgradeBtn`, `VerDowngrade`, `DowngradeBtn`, `ChooseAction`, `InstallBtn`, `PreviewBtn`, `UninstallBtn`

**설정 다이얼로그**: `DlgTitleFmt`, `GrpRendering`, `ChkForceCPU`, `GrpDisplay`, `ChkOverlay`, `ChkClock`, `GrpUpdate`, `BtnCheckUpdate`, `BtnOK`, `BtnCancel`

**업데이트**: `Checking`, `NewVersion`, `BtnDownload`, `UpToDate`, `CheckFailed`

**기타**: `PressF1`, `UnknownCPU`, `UnknownGPU`

콘텐츠 고유 문자열은 콘텐츠가 자체 열거형과 번역 테이블로 관리합니다.

---

## 11. API 레퍼런스: UpdateChecker

헤더: `framework/UpdateChecker.h`

GitHub Releases API를 통한 비동기 업데이트 확인.

### 함수

```cpp
void CheckForUpdateAsync(HWND hwnd,
                         const wchar_t* host,
                         const wchar_t* path,
                         const wchar_t* userAgent);
```

백그라운드 스레드에서 HTTPS GET 요청을 수행하고, 완료 시 `hwnd`에 `WM_UPDATE_RESULT` 메시지를 전달합니다.

### 커스텀 메시지

| 상수 | 값 | 설명 |
|------|------|------|
| `WM_UPDATE_RESULT` | `WM_APP + 100` | 업데이트 확인 결과 메시지 |

### wParam 값

| 상수 | 값 | 설명 |
|------|------|------|
| `UPDATE_NONE` | 0 | 최신 버전 |
| `UPDATE_AVAILABLE` | 1 | 새 버전 있음 (lParam -> `UpdateInfo*`) |
| `UPDATE_ERROR` | 2 | 확인 실패 |

### UpdateInfo 구조체

```cpp
struct UpdateInfo {
    wchar_t version[32];   // 새 버전 문자열
    wchar_t url[512];      // 다운로드 URL
};
```

`lParam`으로 전달되며, 수신측에서 `delete`해야 합니다.

---

## 12. API 레퍼런스: TextOverlay

헤더: `overlay/TextOverlay.h`

화면 좌상단 텍스트 오버레이. 시스템 정보(GPU/CPU/RAM) + 콘텐츠 동적 정보를 표시합니다. 느린 코사인 바운스로 좌우 이동하여 번인을 방지합니다.

| 메서드 | 설명 |
|--------|------|
| `void Init(int renderW, int renderH, int screenW, int screenH, HDC surfDC, const SystemInfo& sysInfo, bool isLite)` | 폰트 생성, 시스템 정보 라인 포맷, 바운스 범위 계산. |
| `void Render(HDC surfDC, float fadeAlpha, const wchar_t* dynamicLine)` | 매 프레임 오버레이 렌더링. `fadeAlpha`로 밝기 조절, `dynamicLine`은 콘텐츠 제공 문자열. |
| `void RenderHelpLine(HDC surfDC, float fadeAlpha)` | F1 안내 문구만 렌더링 (오버레이 꺼져 있어도 표시). |

---

## 13. API 레퍼런스: ClockOverlay

헤더: `overlay/ClockOverlay.h`

7-세그먼트 디지털 시계 오버레이. 반투명 AlphaBlend 합성, 바운스 물리로 번인 방지.

| 메서드 | 설명 |
|--------|------|
| `void Init(int renderW, int renderH)` | 비트맵 및 세그먼트 설정, 바운스 시작 위치/속도 결정. |
| `void Render(HDC surfDC, int renderW, int renderH)` | 현재 시각(HH:MM) 표시 + 바운스 업데이트 + AlphaBlend 합성. |

시각은 분 단위로 캐싱되어 분이 바뀔 때만 비트맵을 재생성합니다.

---

## 14. API 레퍼런스: SystemInfo

헤더: `overlay/SystemInfo.h`

시스템 하드웨어 정보 수집기. 앱 시작 시 한 번만 수집합니다.

| 메서드 | 설명 |
|--------|------|
| `void Collect()` | GPU, CPU, RAM 정보 수집. |
| `const wchar_t* GetGPUName() const` | GPU 이름 반환. |
| `const wchar_t* GetCPUName() const` | CPU 이름 반환. |
| `int GetRAMGB() const` | RAM 용량 (GB) 반환. |

---

## 15. API 레퍼런스: SegmentDisplay

헤더: `render/SegmentDisplay.h`

7-세그먼트 디스플레이 렌더러.

### SegmentDisplayConfig 구조체

| 필드 | 설명 |
|------|------|
| `int digitW` | 숫자 폭 |
| `int digitH` | 숫자 높이 |
| `int segThick` | 세그먼트 두께 |
| `int segGap` | 세그먼트 간격 |
| `int digitSpacing` | 숫자 간 간격 |
| `int colonW` | 콜론 폭 |
| `int dimBrightness` | 꺼진 세그먼트 밝기 |

### 함수

```cpp
void Draw7SegClock(HDC hdc, int bmpW, int bmpH,
                   const SegmentDisplayConfig& cfg,
                   int hour, int minute);
```

비트맵 영역 중앙에 HH:MM 형식의 7-세그먼트 시계를 그립니다.

---

## 16. API 레퍼런스: Window

헤더: `framework/Window.h`

Win32 윈도우 기본 래퍼 클래스. WndProc 썽크로 C++ 가상 함수 디스패치를 지원합니다. `ScreenSaverWindow`와 `PreviewWindow`의 기반 클래스입니다.

| 메서드 | 설명 |
|--------|------|
| `bool Create(HINSTANCE, const wchar_t* className, const wchar_t* title, DWORD style, DWORD exStyle, int x, int y, int w, int h, HWND parent)` | 윈도우 생성. 윈도우 클래스 등록 포함. |
| `void Show(int cmdShow = SW_SHOW)` | 윈도우 표시. |
| `HWND GetHwnd() const` | 윈도우 핸들 반환. |
| `virtual LRESULT HandleMessage(UINT msg, WPARAM wp, LPARAM lp)` | 메시지 처리 가상 함수. 서브클래스에서 오버라이드. |

---

## 17. API 레퍼런스: ScreenSaverWindow

헤더: `framework/ScreenSaverWindow.h`

전체화면 스크린세이버 윈도우. 마우스/키보드 입력 시 종료, F1으로 설정 다이얼로그, 다중 모니터 지원.

### 초기화

```cpp
bool Init(HINSTANCE hInst, bool windowed = false, int emuMonitors = 0);
```

- `windowed`: `true`이면 데스크톱 크기 윈도우 모드 (디버깅용)
- `emuMonitors`: `> 0`이면 다중 모니터 에뮬레이션 (windowed 강제)

모니터 열거 -> 프라이머리(가장 큰 면적) 선택 -> 전체화면 창 생성 -> 보조 모니터에 미러 창 생성.

### 렌더링

| 메서드 | 설명 |
|--------|------|
| `void SetRenderCallback(RenderCallback cb)` | 렌더 콜백 설정. `void(HDC, int w, int h)`. |
| `void RequestRedraw()` | `InvalidateRect` -> `WM_PAINT` -> 렌더 콜백 호출. |
| `void UpdateMirrorWindows(HDC sourceDC, int sourceW, int sourceH)` | 보조 모니터에 렌더 결과 복사 (종횡비 보존 중앙 크롭). |

### 크기

| 메서드 | 설명 |
|--------|------|
| `int GetWidth() const` | 프라이머리 모니터 폭. |
| `int GetHeight() const` | 프라이머리 모니터 높이. |
| `int GetMonitorCount() const` | 총 모니터 수. |

### 설정 다이얼로그 연동

| 메서드 | 설명 |
|--------|------|
| `bool IsConfigRequested() const` | F1 키가 눌렸는지 확인. |
| `void ClearConfigRequest()` | 설정 요청 플래그 초기화. |
| `void SetConfigDialogOpen(bool open)` | 다이얼로그 열림 상태 설정 (입력 무시용). |
| `void ResetInputTracking()` | 마우스 초기 위치 리셋 (다이얼로그 닫은 후 사용). |

### 입력 처리

- 시작 후 2초간 입력 무시 (grace period)
- 마우스 5px 이상 이동 시 종료
- 키보드/클릭 시 종료
- F1: 설정 다이얼로그 열기
- 같은 프로세스 PID의 미러 창에 의한 `WM_ACTIVATE` 비활성화 무시

### MonitorInfo 구조체

```cpp
struct MonitorInfo {
    RECT rect;     // 모니터 영역 (가상 스크린 좌표)
    int width;     // 모니터 폭
    int height;    // 모니터 높이
};
```

---

## 18. API 레퍼런스: PreviewWindow

헤더: `framework/PreviewWindow.h`

제어판 미리보기 윈도우. 부모 HWND의 자식 윈도우로 생성됩니다.

| 메서드 | 설명 |
|--------|------|
| `bool Init(HINSTANCE hInst, HWND parentHwnd)` | 부모 창 안에 자식 윈도우 생성. |
| `void SetRenderCallback(RenderCallback cb)` | 렌더 콜백 설정. |
| `void RequestRedraw()` | 화면 갱신 요청. |
| `int GetWidth() const` | 미리보기 창 폭. |
| `int GetHeight() const` | 미리보기 창 높이. |

---

## 19. API 레퍼런스: ConfigDialog

헤더: `framework/ConfigDialog.h`

설정 다이얼로그. 프레임워크 공통 컨트롤을 자동 관리하고, 콘텐츠 고유 컨트롤은 `IScreenSaverContent`에 위임합니다.

```cpp
static bool Show(HINSTANCE hInst, HWND parent,
                 const AppDescriptor& desc,
                 FrameworkSettings& settings,
                 IScreenSaverContent* content = nullptr);
```

- `parent`: 부모 윈도우 (`nullptr`이면 데스크톱)
- `settings`: 프레임워크 설정 (입출력)
- `content`: 콘텐츠 포인터 (다이얼로그 초기화/읽기 위임)
- 반환값: `true`이면 OK로 닫힘, `false`이면 취소

---

## 20. API 레퍼런스: ControlIds

헤더: `framework/ControlIds.h`

설정 다이얼로그에서 프레임워크가 사용하는 표준 컨트롤 ID. 앱의 리소스 파일(.rc)에서 동일한 ID를 사용합니다.

| ID | 값 | 설명 |
|------|------|------|
| `IDC_GRP_RENDERING` | 1020 | 렌더링 그룹박스 |
| `IDC_FORCE_CPU` | 1006 | "Force CPU rendering" 체크박스 |
| `IDC_GRP_DISPLAY` | 1022 | 디스플레이 그룹박스 |
| `IDC_SHOW_OVERLAY` | 1008 | "Show system info" 체크박스 |
| `IDC_SHOW_CLOCK` | 1009 | "Show clock" 체크박스 |
| `IDC_GRP_UPDATE` | 1023 | 업데이트 그룹박스 |
| `IDC_UPDATE_CHECK` | 1010 | "Check for Update" 버튼 |
| `IDC_UPDATE_STATUS` | 1011 | 업데이트 상태 텍스트 |
| `IDC_VERSION_LABEL` | 1012 | 버전 표시 레이블 |

모든 컨트롤은 선택적입니다. 다이얼로그에 해당 ID의 컨트롤이 없으면 프레임워크가 자동으로 건너뜁니다.

앱 고유 컨트롤 ID는 1100번대 이상을 사용하세요.

---

## 21. 설정 다이얼로그 가이드

### 구조

설정 다이얼로그는 앱의 RC 파일에서 정의합니다. 프레임워크 표준 컨트롤 ID를 사용하면 프레임워크가 자동으로:
- `Localization`을 통해 텍스트를 현재 언어로 교체
- `FrameworkSettings`와 체크박스 상태를 동기화
- `AppDescriptor` 기능 플래그에 따라 컨트롤 표시/숨김

### 프레임워크 자동 처리 흐름

```
다이얼로그 생성 (WM_INITDIALOG)
  |-- [다국어] 제목, 그룹박스, 체크박스, 버튼 텍스트 교체
  |-- [설정] 체크박스 상태 설정 (forceCPU, showOverlay, showClock)
  |-- [기능] hasOverlay=false -> 관련 컨트롤 숨김
  |-- [기능] hasClock=false -> 관련 컨트롤 숨김
  |-- [기능] hasAutoUpdate=false -> 관련 컨트롤 숨김
  |-- [버전] IDC_VERSION_LABEL에 파일 버전 표시
  +-- content->InitConfigControls(dlg)   // 콘텐츠 고유 컨트롤 초기화

OK 클릭
  |-- [설정] 체크박스 -> FrameworkSettings에 반영
  |-- Registry::SaveFrameworkSettings()
  +-- content->ReadConfigControls(dlg)   // 콘텐츠 고유 값 읽기/저장
```

### 콘텐츠 고유 컨트롤 추가

1. `resource.h`에 컨트롤 ID 정의 (1100번대 이상)
2. `resource.rc`에 컨트롤 배치
3. `InitConfigControls()`에서 컨트롤 초기화
4. `ReadConfigControls()`에서 값 읽기 및 저장
5. (필요 시) `HandleConfigMessage()`에서 실시간 상호작용 처리

```cpp
void MyContent::InitConfigControls(HWND dlg) {
    SendDlgItemMessage(dlg, IDC_MY_SLIDER, TBM_SETRANGE, TRUE, MAKELPARAM(1, 100));
    SendDlgItemMessage(dlg, IDC_MY_SLIDER, TBM_SETPOS, TRUE, speed_);
}

void MyContent::ReadConfigControls(HWND dlg) {
    speed_ = (int)SendDlgItemMessage(dlg, IDC_MY_SLIDER, TBM_GETPOS, 0, 0);
    SaveSettings();
}

bool MyContent::HandleConfigMessage(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_HSCROLL) {
        // 슬라이더 실시간 반응
        return true;
    }
    return false;
}
```

---

## 22. 비동기 렌더링 가이드

### 동기 렌더링 (기본)

가장 간단한 구현. `BeginRender()`에서 모든 작업을 완료합니다:

```cpp
void MyContent::BeginRender() {
    rendering_ = true;
    // 전체 렌더링 작업
    RenderFrame();
    rendering_ = false;
}

bool MyContent::IsRenderComplete() const { return !rendering_; }
void MyContent::FinalizeRender() { /* 추가 작업 없음 */ }
```

### 비동기 렌더링 (별도 스레드)

렌더링이 오래 걸리는 경우 별도 스레드에서 실행합니다:

```cpp
void MyContent::BeginRender() {
    rendering_ = true;
    renderThread_ = std::thread([this]() {
        HeavyRender();
        rendering_ = false;
    });
    renderThread_.detach();
}

bool MyContent::IsRenderComplete() const { return !rendering_; }

void MyContent::FinalizeRender() {
    // 렌더 스레드가 쓴 결과를 surface_에 반영
    // 예: 더블 버퍼링에서 백 버퍼 -> 프론트 버퍼 복사
}
```

### 프레임워크 동작

- `IsRenderComplete() == false`인 동안 이전 프레임을 계속 표시
- CPU 렌더링 시 30fps 프레임 캡 적용
- GPU 렌더링 시 프레임 캡 없음 (VSync 의존)

---

## 23. 시간 보간 가이드

비동기 렌더링 대기 중 이전 프레임을 확대/축소하여 부드러운 전환을 제공합니다. 줌인 애니메이션(프랙탈 등)에 적합합니다.

### 활성화

```cpp
bool MyContent::SupportsInterpolation() const { return true; }

IScreenSaverContent::InterpInfo MyContent::GetInterpInfo() const {
    InterpInfo info;
    info.prevScale = lastRenderedScale_;    // FinalizeRender 시점 스냅샷
    info.currScale = currentAnimScale_;     // 현재 애니메이션 값
    info.prevCenterX = lastRenderedCX_;
    info.prevCenterY = lastRenderedCY_;
    info.currCenterX = currentCX_;
    info.currCenterY = currentCY_;
    info.aspectRatio = static_cast<float>(width_) / height_;
    return info;
}
```

### 프레임워크 보간 로직

1. `FinalizeRender()` 시 이전 프레임을 백업 (`BitBlt`)
2. `InterpInfo`에서 `prevScale / currScale` 비율 계산
3. 비율이 1.001보다 크면 (줌인 중이면) `StretchBlt`로 이전 프레임 확대
4. 60fps로 보간 프레임 출력
5. 새 렌더가 완료되면 실제 프레임으로 교체

---

## 24. 다중 모니터 가이드

### 자동 동작

`ScreenSaverWindow::Init()`이 `EnumDisplayMonitors()`로 모니터를 열거하고:
- 면적이 가장 큰 모니터를 프라이머리(렌더 대상)로 선택
- 나머지 모니터에 미러 창(WS_POPUP | WS_EX_TOPMOST) 생성
- 미러 창에 종횡비 보존 중앙 크롭으로 렌더 결과 복사

콘텐츠 구현에서는 다중 모니터를 의식할 필요가 없습니다.

### 렌더 순서와 오버레이 분리

렌더 콜백에서 보조 모니터 복사는 오버레이/시계 합성 **이전에** 수행됩니다:

```
[렌더 콜백]
  1. UpdateMirrorWindows(surfDC)  -- 순수 콘텐츠를 보조 모니터에 복사
  2. ClockOverlay::Render()       -- 프라이머리 surfDC에 시계 합성
  3. TextOverlay::Render()        -- 프라이머리 surfDC에 오버레이 합성
  4. content->BlitTo(hdc)         -- 프라이머리 화면에 출력
```

결과적으로:
- **프라이머리 모니터**: 콘텐츠 + 오버레이 + 시계
- **보조 모니터**: 순수 콘텐츠만 (오버레이/시계 없음)

### 미러링 방식

소스(프라이머리)와 대상(보조)의 종횡비가 다를 때:
- 대상이 더 넓으면 -> 소스의 위/아래 크롭
- 대상이 더 좁으면 -> 소스의 좌/우 크롭

### 에뮬레이션 모드

단일 모니터에서 다중 모니터 동작을 테스트:

```bat
MyApp.scr /emu        # 듀얼 모니터 (16:9 + 4:3)
MyApp.scr /emu:3      # 트리플 모니터
MyApp.scr /emu:4      # 쿼드 모니터
```

에뮬레이션은 windowed로 강제되고, non-topmost이며, ESC만으로 종료됩니다(디버깅 편의).

종횡비 프리셋 (크롭 로직 테스트용):
- 프라이머리: 16:9
- 보조 1: 4:3
- 보조 2: 21:9
- 보조 3: 16:10

---

## 25. 커맨드라인 레퍼런스

`ScreenSaverEngine::Run()`이 `GetCommandLineW()`를 파싱합니다. `/`와 `-` 접두사 모두 지원.

### 모드 선택

| 인자 | 동작 |
|------|------|
| `/s` | 전체화면 스크린세이버 |
| `/w` | `/s`와 동일 |
| `/c` | 설정 다이얼로그 |
| `/p <hwnd>` | 제어판 미리보기 (hwnd는 부모 창 핸들, 10진수) |
| `/i` 또는 `/install` | System32에 설치 |
| `/u` | 설치 제거 |
| (없음, .exe) | 런처 다이얼로그 (설치/미리보기/제거 선택) |
| (없음, .scr) | 설정 다이얼로그 |

### 렌더링 모드

| 인자 | 동작 |
|------|------|
| `/cpu` | CPU 렌더링 강제 (레지스트리 무시) |
| `/gpu` | GPU 렌더링 강제 (레지스트리 무시) |

### 다중 모니터

| 인자 | 동작 |
|------|------|
| `/emu` | 듀얼 모니터 에뮬레이션 |
| `/emu:N` | N개 모니터 에뮬레이션 (N=2~4) |

`/emu`를 단독으로 사용하면 자동으로 스크린세이버 모드(`/s`)로 진입합니다.

### 기타

| 인자 | 동작 |
|------|------|
| `/lang:xx` | 언어 오버라이드 (en, ko, ja, zh, de, fr, es) |
| `/screenshot:N` | N초 간격 PNG 스크린샷 저장 (실행 파일 위치의 screenshots/ 폴더) |
| `/screenshot:N:폴더` | 지정 폴더에 저장 |
| `/silent` | 메시지 박스 숨김 (자동 설치/제거에 사용) |

### 조합 예시

```bat
rem GPU 강제 + 한국어 + 스크린세이버 모드
MyApp.scr /s /gpu /lang:ko

rem CPU 강제 + 5초 간격 스크린샷
MyApp.scr /s /cpu /screenshot:5

rem 트리플 모니터 에뮬레이션
MyApp.scr /emu:3

rem 자동 설치 (메시지 박스 없음)
MyApp.scr /install /silent
```

---

## 26. 링크 의존성

`wsse.lib`를 링크할 때 필요한 시스템 라이브러리:

| 라이브러리 | 용도 |
|-----------|------|
| `user32.lib` | 윈도우, 메시지, 다이얼로그, SystemParametersInfo |
| `gdi32.lib` | GDI 렌더링, DIB 섹션, BitBlt/StretchBlt |
| `gdiplus.lib` | PNG 저장 (RenderSurface::SavePNG) |
| `shell32.lib` | ShellExecuteW (설치, 브라우저 열기) |
| `advapi32.lib` | 레지스트리, 토큰 조회 (관리자 권한) |
| `comctl32.lib` | 공용 컨트롤, TaskDialog (런처) |
| `winhttp.lib` | HTTPS (자동 업데이트) |
| `msimg32.lib` | AlphaBlend (시계 오버레이) |
| `comdlg32.lib` | 공용 다이얼로그 |
| `shlwapi.lib` | 경로 유틸리티 (자동 링크: `#pragma comment`) |
| `version.lib` | 파일 버전 조회 (자동 링크: `#pragma comment`) |

> `hasAutoUpdate = false`이면 런타임에 `winhttp.lib`를 사용하지 않지만, `wsse.lib`가 심볼을 포함하고 있어 링커가 요구할 수 있으므로 포함을 권장합니다.
