# WindowsScreenSaverEngine 변경 이력

## 2026-03-27: 인터랙티브 모드, hasGPU 기반 GPU 사용률 표시 제어

### 작업 내용
- IScreenSaverContent: IsInteractive(), OnClick() 가상 메서드 추가
- ScreenSaverWindow: 인터랙티브 모드 마우스 클릭/우클릭 처리
- ScreenSaverEngine: hasGPU=false일 때 GPU 사용률 측정(Init/Update)/표시 생략
- TextOverlay: kMaxCharsEstimate 120으로 조정

### 수정 파일
- `src/framework/IScreenSaverContent.h` - IsInteractive(), OnClick() 추가
- `src/framework/ScreenSaverEngine.cpp` - hasGPU 조건부 GPU 사용률 처리
- `src/framework/ScreenSaverWindow.h` - interactiveMode_, clickPending_, exitRequested_
- `src/framework/ScreenSaverWindow.cpp` - 인터랙티브 모드 마우스 처리
- `src/overlay/TextOverlay.cpp` - kMaxCharsEstimate 조정

## 2026-03-22: AppDescriptor hasGPU 플래그, /cpu /gpu 인자 제거

### 작업 내용
- AppDescriptor에 hasGPU 플래그 추가 (기본값 true, 기존 호환)
- ConfigDialog: hasGPU=false일 때 렌더링 섹션(IDC_GRP_RENDERING, IDC_FORCE_CPU) 숨김
- ScreenSaverEngine: ParseMode에서 /cpu, /gpu 커맨드라인 인자 파싱 제거
- ScreenSaverEngine: renderOverride 변수 및 관련 로직 제거
- ScreenSaverEngine: 설정 변경 후 frameCap = !IsUsingGPU() 제거 (항상 frameCap = true)

### 수정 파일
- `src/framework/AppDescriptor.h` - hasGPU 플래그 추가
- `src/framework/ConfigDialog.cpp` - hasGPU=false 처리 추가
- `src/framework/ScreenSaverEngine.cpp` - /cpu, /gpu 파싱 제거, renderOverride 제거

## 2026-03-21: 디버그 로그 제거, 오버레이 레이아웃 변경, GPU 부하 표시, 이름 축약

### 작업 내용
- 디버그 로그 코드 완전 제거 (cstdio, g_logFile, LogInit, Log, LogClose)
- TextOverlay 레이아웃 변경: 4줄(CPU/RAM/GPU/F1) -> 3줄(CPU+RAM/GPU+해상도/F1)
  - cpuLine_: "CPU: <이름> | RAM: N GB", gpuLine_: "GPU: <이름> | WxH"
  - CPU 부하를 " | RAM:" 앞에 삽입, GPU 부하를 " | " 앞에 삽입
- GPU 부하 표시: IScreenSaverContent::GetGPULoad(), TextOverlay::Render gpuLoad 파라미터
- Localization: GpuInUse 문자열 7개 언어 추가
- SystemInfo: CPU/GPU 이름 축약 (상표, 브랜드명 제거)

### 수정 파일
- `src/framework/ScreenSaverEngine.cpp` - 디버그 로그 제거, gpuLoad 전달
- `src/framework/IScreenSaverContent.h` - GetGPULoad() 가상 함수 추가
- `src/framework/Localization.h` - GpuInUse 열거값 추가
- `src/framework/Localization.cpp` - GpuInUse 7개 언어 번역 추가
- `src/overlay/TextOverlay.h` - cpuLine_/gpuLine_ 멤버, Render gpuLoad 파라미터
- `src/overlay/TextOverlay.cpp` - 3줄 레이아웃, CPU/GPU 부하 삽입 로직
- `src/overlay/SystemInfo.cpp` - ShortenCPUName/ShortenGPUName 함수, shlwapi.h

## 2026-03-09: 보조 모니터 오버레이 분리

### 작업 내용
- 렌더 콜백에서 UpdateMirrorWindows() 호출 순서를 오버레이/시계 합성 이전으로 변경
- 보조 모니터에는 순수 콘텐츠만 표시 (오버레이/시계는 프라이머리에만 합성)

### 수정 파일
- `src/framework/ScreenSaverEngine.cpp` - 렌더 콜백 내 UpdateMirrorWindows 호출 순서 변경

## 2026-03-09: 다중 모니터 지원 + 에뮬레이션

### 작업 내용
- EnumDisplayMonitors로 모니터 열거, 면적 기준 프라이머리(렌더) 모니터 선택
- 보조 모니터에 미러 창(MirrorWndProc) 생성: WS_POPUP | WS_VISIBLE | WS_EX_TOPMOST
- 보조 창 입력 처리: 키/클릭 종료, 마우스 이동 5px 이상 시 종료, grace period 2초
- UpdateMirrorWindows(): surfDC에서 StretchBlt로 종횡비 보존 중앙 크롭 복사
- WM_ACTIVATE: 같은 프로세스 PID 확인으로 미러 창에 의한 비활성화 무시
- ResetInputTracking(): 보조 창 마우스 상태도 함께 리셋
- 단일 모니터에서는 기존 동작과 완전히 동일
- 에뮬레이션 모드: /emu (듀얼) 또는 /emu:N (N=2~4)
  - GenerateEmuMonitors(): 화면 크기 기반 가상 모니터 배치 생성
  - 종횡비 프리셋: 16:9, 4:3, 21:9, 16:10 (크롭 로직 테스트용)
  - windowed 강제, non-topmost, ESC만 종료 (디버깅용)
  - /emu 단독 사용 시 자동 ScreenSaver 모드 진입

### 수정 파일
- `src/framework/ScreenSaverWindow.h` - MonitorInfo, MirrorState, Init 시그니처, emuMode_
- `src/framework/ScreenSaverWindow.cpp` - 모니터 열거/에뮬레이션, 미러 창 생성/파괴/렌더링
- `src/framework/ScreenSaverEngine.h` - emuMonitors_ 멤버
- `src/framework/ScreenSaverEngine.cpp` - /emu 파싱, 렌더 콜백에 UpdateMirrorWindows

## 2026-03-08: 프로젝트 초기 생성

Fractal 프로젝트의 프레임워크 코드를 분리하여 독립 프로젝트로 생성.

### 작업 내용
- Fractal/src/framework/ 전체를 복사하여 독립 프로젝트 구성
- Fractal/src/overlay/ (SystemInfo, TextOverlay, ClockOverlay) 포함
- Fractal/src/render/ 중 RenderSurface, SegmentDisplay 포함
- 네임스페이스 `fractal` -> `wsse` 변경
- `ConfigDialog.cpp`의 `../res/resource.h` 의존성 제거 -> `framework/ControlIds.h` 신규 생성
- 윈도우 클래스명 범용화: `FractalSaverWnd` -> `WSSEScreenSaverWnd`, `FractalPreviewWnd` -> `WSSEPreviewWnd`
- 정적 라이브러리(`wsse.lib`) 빌드 스크립트 작성

### 파일 목록
- `src/framework/` (13 .h + 8 .cpp): AppDescriptor, ConfigDialog, ControlIds, FrameworkSettings, IPreviewContent, IScreenSaverContent, Localization, PreviewWindow, Registry, ScreenSaverEngine, ScreenSaverWindow, UpdateChecker, Window
- `src/overlay/` (3 .h + 3 .cpp): ClockOverlay, SystemInfo, TextOverlay
- `src/render/` (2 .h + 2 .cpp): RenderSurface, SegmentDisplay
- `build.bat`: MSVC 정적 라이브러리 빌드
