# WindowsScreenSaverEngine (wsse)

Windows 스크린세이버 개발을 위한 C++ 정적 라이브러리 프레임워크입니다.

윈도우 관리, 메시지 루프, 커맨드라인 파싱, 설정 다이얼로그, 오버레이, 시계, 스크린샷, 다중 모니터, 설치/제거, 자동 업데이트 등 스크린세이버의 공통 기능을 제공하며, 개발자는 `IScreenSaverContent` 인터페이스 하나만 구현하면 완전한 스크린세이버를 만들 수 있습니다.

## 특징

- **콘텐츠 분리 아키텍처** -- `IScreenSaverContent` 인터페이스만 구현하면 완전한 스크린세이버 완성
- **비동기 렌더링** -- 프레임워크가 프레임 타이밍과 렌더 스케줄링 관리
- **시간 보간** -- 비동기 렌더링 중 이전 프레임을 확대/축소하여 부드러운 전환
- **다중 모니터** -- 프라이머리 모니터에서 렌더링, 보조 모니터에 종횡비 보존 미러링
- **다중 모니터 에뮬레이션** -- `/emu` 또는 `/emu:N`으로 단일 모니터에서 다중 모니터 테스트
- **7-세그먼트 디지털 시계** -- 바운스 물리 기반 LED 스타일 시계 오버레이
- **시스템 정보 오버레이** -- CPU/GPU/해상도 정보 + 콘텐츠 동적 라인
- **다국어 지원** -- 7개 언어 (EN, KO, JA, ZH, DE, FR, ES), OS 로케일 자동 감지
- **자동 업데이트** -- GitHub Releases API 연동
- **설치/제거** -- System32에 .scr 등록, 관리자 권한 자동 요청
- **설정 다이얼로그** -- 프레임워크 공통 컨트롤 자동 관리 + 콘텐츠 고유 컨트롤 확장
- **레지스트리 관리** -- 설정 읽기/쓰기 유틸리티
- **스크린샷** -- BMP/PNG 저장 (GDI+)
- **미리보기** -- 제어판 미리보기 창(/p) 지원
- **외부 의존성 없음** -- Win32 API + 표준 C++20만 사용

## 빌드 요구사항

- MSVC BuildTools 2022 이상
- Windows SDK 10.0.26100.0 이상
- C++20

## 빌드

```bat
cmd.exe /c build.bat
```

`lib/Release/wsse.lib` 정적 라이브러리가 생성됩니다.

빌드 옵션: `/O2 /GL /MT /W4 /EHsc /std:c++20 /fp:fast` + LTCG

## 디렉토리 구조

```
WindowsScreenSaverEngine/
  src/
    framework/          엔진 핵심
      ScreenSaverEngine   엔진 메인 (라이프사이클 관리)
      IScreenSaverContent 콘텐츠 인터페이스 (구현 필수)
      IPreviewContent     미리보기 콘텐츠 인터페이스
      AppDescriptor       앱 메타데이터 및 기능 플래그
      FrameworkSettings   공통 설정 (forceCPU, showOverlay, showClock)
      ScreenSaverWindow   전체화면 윈도우 + 다중 모니터
      PreviewWindow       미리보기 윈도우
      ConfigDialog        설정 다이얼로그
      ControlIds          프레임워크 표준 컨트롤 ID
      Window              Win32 윈도우 기반 클래스
      Registry            레지스트리 유틸리티
      Localization        다국어 번역 (7개 언어)
      UpdateChecker       GitHub Releases 자동 업데이트
    overlay/            화면 오버레이
      TextOverlay         텍스트 오버레이 렌더링
      SystemInfo          CPU/GPU/해상도 정보 수집
      ClockOverlay        7-세그먼트 시계 (바운스 물리)
    render/             렌더링 유틸리티
      RenderSurface       DIB 섹션 래퍼 (픽셀 버퍼)
      SegmentDisplay      7-세그먼트 디스플레이 렌더러
  lib/Release/
    wsse.lib            빌드 산출물
  build.bat             빌드 스크립트
  MANUAL.md             프레임워크 매뉴얼 (API 레퍼런스)
  QUICKSTART.md         퀵스타트 가이드 (튜토리얼)
```

## 빠른 시작

최소 3개 파일로 스크린세이버를 만들 수 있습니다:

### 1. 콘텐츠 구현

```cpp
#include "framework/IScreenSaverContent.h"
#include "render/RenderSurface.h"

class MyContent : public wsse::IScreenSaverContent {
    wsse::RenderSurface surface_;

public:
    void Init(int w, int h, bool forceCPU) override {
        surface_.Create(w, h);
    }

    void BeginRender() override { /* 비동기 렌더링 시작 */ }
    bool IsRenderComplete() const override { return true; }
    void FinalizeRender() override { /* 결과 반영 */ }
    bool IsRendering() const override { return false; }

    HDC GetSurfaceDC() const override { return surface_.GetMemDC(); }
    const wsse::RenderSurface& GetSurface() const override { return surface_; }
    void BlitTo(HDC hdc, int dw, int dh) override {
        surface_.StretchBlitTo(hdc, 0, 0, dw, dh);
    }

    void Update(double dt) override { /* 애니메이션 갱신 */ }
    float GetFadeAlpha() const override { return 1.0f; }
    bool IsUsingGPU() const override { return false; }
};
```

### 2. 엔트리 포인트

```cpp
#include "framework/ScreenSaverEngine.h"
#include "MyContent.h"

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int) {
    wsse::AppDescriptor desc;
    desc.scrName     = L"MyApp.scr";
    desc.appTitle    = L"My Screen Saver";
    desc.registryKey = L"Software\\MyApp";
    desc.dialogResourceId = IDD_CONFIG;

    auto content = std::make_unique<MyContent>();
    wsse::ScreenSaverEngine engine(desc, std::move(content));
    return engine.Run(hInst);
}
```

### 3. 링크

```bat
cl /I"path/to/wsse/src" ... your_sources.cpp /link wsse.lib ^
    user32.lib gdi32.lib gdiplus.lib shell32.lib advapi32.lib ^
    comctl32.lib winhttp.lib msimg32.lib comdlg32.lib
```

자세한 사용법은 [QUICKSTART.md](QUICKSTART.md)와 [MANUAL.md](MANUAL.md)를 참조하세요.

## 커맨드라인 인터페이스

프레임워크가 자동으로 파싱하는 커맨드라인 인자:

| 인자 | 동작 |
|------|------|
| `/s` | 전체화면 스크린세이버 실행 |
| `/c` | 설정 다이얼로그 |
| `/p <hwnd>` | 제어판 미리보기 |
| `/i` 또는 `/install` | System32에 설치 (관리자 권한 자동 요청) |
| `/u` | 설치 제거 (관리자 권한 자동 요청) |
| `/emu` | 듀얼 모니터 에뮬레이션 |
| `/emu:N` | N개 모니터 에뮬레이션 (N=2~4) |
| `/cpu` | CPU 렌더링 강제 |
| `/gpu` | GPU 렌더링 강제 |
| `/lang:xx` | 언어 오버라이드 (en, ko, ja, zh, de, fr, es) |
| `/screenshot:N[:폴더]` | N초 간격 PNG 스크린샷 자동 저장 |
| `/silent` | 메시지 박스 숨김 (자동 설치/제거용) |
| (없음) | .exe이면 런처 다이얼로그, .scr이면 설정 다이얼로그 |

## 네임스페이스

모든 API는 `wsse` 네임스페이스에 정의되어 있습니다.

## 라이선스

비공개 프로젝트
