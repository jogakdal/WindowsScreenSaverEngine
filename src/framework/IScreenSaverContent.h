#pragma once
#include <windows.h>
#include <memory>
#include "framework/IPreviewContent.h"

namespace wsse {

class RenderSurface;

// 스크린세이버 콘텐츠 인터페이스
//
// 스크린세이버의 렌더링, 애니메이션, 설정을 구현하는 핵심 인터페이스.
// 프레임워크(ScreenSaverEngine)가 윈도우, 메시지 루프, 오버레이, 시계,
// 스크린샷, 설치/제거, 자동 업데이트를 담당하고,
// 콘텐츠 구현체는 실제 화면에 표시되는 내용만 책임집니다.
//
// 구현 시 순수 가상 함수만 필수이며, 나머지는 기본 동작을 제공합니다.
//
// 사용 예:
//   class MyContent : public IScreenSaverContent {
//       void Init(int w, int h, bool forceCPU) override { /* ... */ }
//       void BeginRender() override { /* 비동기 렌더 시작 */ }
//       bool IsRenderComplete() const override { return true; }
//       // ... 기타 순수 가상 함수 구현
//   };
class IScreenSaverContent {
public:
    virtual ~IScreenSaverContent() = default;

    // ================================================================
    // 초기화
    // ================================================================

    // 렌더러 초기화
    // renderW, renderH: 렌더링 해상도
    // forceCPU: true이면 GPU 사용 불가 (CPU 폴백)
    virtual void Init(int renderW, int renderH, bool forceCPU) = 0;

    // 화면 크기에서 실제 렌더링 크기 결정
    // Lite 버전 등에서 축소 렌더링 시 오버라이드
    // 기본: 화면 크기 그대로 사용
    virtual void GetRenderSize(int screenW, int screenH,
                               int& renderW, int& renderH) const {
        renderW = screenW;
        renderH = screenH;
    }

    // Lite(경량) 버전 여부 -- 오버레이에 해상도 정보 표시에 사용
    virtual bool IsLite() const { return false; }

    // ================================================================
    // 비동기 렌더링
    // ================================================================

    // 새 프레임 렌더링 시작 (내부 상태 기반)
    // 비동기: 호출 즉시 반환, IsRenderComplete()로 완료 확인
    virtual void BeginRender() = 0;

    // 현재 렌더링이 완료되었는지 확인
    virtual bool IsRenderComplete() const = 0;

    // 완료된 렌더링 결과를 표면에 반영
    // IsRenderComplete() == true일 때만 호출 가능
    virtual void FinalizeRender() = 0;

    // 렌더링 진행 중 여부 (BeginRender 호출 후 FinalizeRender 전)
    virtual bool IsRendering() const = 0;

    // ================================================================
    // 출력
    // ================================================================

    // 렌더 표면의 메모리 DC (오버레이 합성용)
    virtual HDC GetSurfaceDC() const = 0;

    // 렌더 표면 참조 (스크린샷 저장용)
    virtual const RenderSurface& GetSurface() const = 0;

    // 렌더 결과를 대상 DC에 복사 (화면 출력)
    // destW, destH: 대상 영역 크기 (렌더 크기와 다를 수 있음)
    virtual void BlitTo(HDC hdc, int destW, int destH) = 0;

    // ================================================================
    // 애니메이션
    // ================================================================

    // 매 프레임 애니메이션 상태 갱신
    // dt: 이전 프레임으로부터 경과 시간 (초)
    virtual void Update(double dt) = 0;

    // 현재 페이드 알파 (0.0 ~ 1.0)
    // 프레임워크가 오버레이/시계 밝기 조절에 사용
    virtual float GetFadeAlpha() const = 0;

    // 현재 페이드아웃 진행 중 여부
    // 프레임워크가 렌더 스케줄링 최적화에 사용
    virtual bool IsFadingOut() const { return false; }

    // GPU 렌더링 사용 여부
    virtual bool IsUsingGPU() const = 0;

    // 스타일/애니메이션 전체 순환 완료 횟수
    // 스크린샷 모드에서 1회 순환 완료 시 종료 조건으로 사용
    virtual int GetCycleCount() const { return 0; }

    // ================================================================
    // 페이드 최적화 (선택)
    // ================================================================

    // 기존 렌더 결과에 페이드 알파만 재적용 가능 여부
    // 프랙탈 재계산 없이 페이드 전환을 빠르게 처리
    virtual bool CanReapplyFade() const { return false; }

    // 기존 렌더 결과에 지정 알파 재적용
    virtual void ReapplyFade(float alpha) { (void)alpha; }

    // ================================================================
    // 시간 보간 (선택)
    // ================================================================

    // 비동기 렌더링 중 이전 프레임을 확대/축소하여 부드러운 전환을 제공하는
    // 줌 보간 기능. SupportsInterpolation()이 true를 반환하면 프레임워크가
    // GetInterpInfo()로 이전/현재 스케일을 조회하여 StretchBlt로 보간합니다.
    struct InterpInfo {
        double prevScale = 0;        // 이전 렌더 프레임의 스케일
        double currScale = 0;        // 현재 애니메이션 스케일
        double prevCenterX = 0;      // 이전 렌더 프레임의 중심 X
        double prevCenterY = 0;      // 이전 렌더 프레임의 중심 Y
        double currCenterX = 0;      // 현재 애니메이션 중심 X
        double currCenterY = 0;      // 현재 애니메이션 중심 Y
        float aspectRatio = 1.0f;    // 가로/세로 비율
    };

    virtual bool SupportsInterpolation() const { return false; }

    // 현재 보간에 필요한 정보 반환
    // prevScale/Center: FinalizeRender 시점의 스냅샷
    // currScale/Center: 현재 애니메이션 상태
    virtual InterpInfo GetInterpInfo() const { return {}; }

    // ================================================================
    // 설정
    // ================================================================

    // 레지스트리에서 콘텐츠 고유 설정 로드
    // Registry::ReadInt/ReadBool 등을 사용하여 직접 읽기
    virtual void LoadSettings() {}

    // 레지스트리에 콘텐츠 고유 설정 저장
    virtual void SaveSettings() {}

    // 프레임워크 설정(forceCPU) 변경 적용
    // 반환값: true이면 렌더 파이프라인 재초기화 필요
    virtual bool ApplyFrameworkSettings(int renderW, int renderH, bool forceCPU) {
        (void)renderW; (void)renderH; (void)forceCPU;
        return false;
    }

    // ================================================================
    // 설정 다이얼로그
    // ================================================================

    // 다이얼로그 초기화 시 콘텐츠 고유 컨트롤 설정
    // 다이얼로그 제목, 콘텐츠별 그룹박스/컨트롤 텍스트 및 값 설정
    virtual void InitConfigControls(HWND dlg) { (void)dlg; }

    // OK 클릭 시 콘텐츠 고유 컨트롤에서 값 읽기 및 저장
    virtual void ReadConfigControls(HWND dlg) { (void)dlg; }

    // 다이얼로그 메시지 처리 (콘텐츠 고유 컨트롤 이벤트)
    // WM_HSCROLL, WM_COMMAND 등 콘텐츠 컨트롤의 상호작용 처리
    // 반환값: true이면 처리 완료 (프레임워크가 추가 처리하지 않음)
    virtual bool HandleConfigMessage(HWND dlg, UINT msg, WPARAM wp, LPARAM lp) {
        (void)dlg; (void)msg; (void)wp; (void)lp;
        return false;
    }

    // ================================================================
    // 스크린샷
    // ================================================================

    // F2 수동 스크린샷 파일명 생성 (확장자 제외)
    // 기본: 타임스탬프 기반
    virtual void FormatScreenshotName(wchar_t* buf, int bufSize) const {
        SYSTEMTIME st;
        GetLocalTime(&st);
        _snwprintf_s(buf, bufSize, _TRUNCATE,
                     L"screenshot_%04d%02d%02d_%02d%02d%02d",
                     st.wYear, st.wMonth, st.wDay,
                     st.wHour, st.wMinute, st.wSecond);
    }

    // ================================================================
    // 오버레이
    // ================================================================

    // 동적 정보 라인 포맷 (시스템 정보 아래 표시)
    // fps: 초당 프레임 수
    // CPU 사용률은 프레임워크가 CPU 라인에 직접 표시
    // 예: "GPU | Zoom: 10^12.3 | FPS: 30 | Smooth | #42"
    virtual void FormatOverlayLine(wchar_t* buf, int bufSize, double fps) {
        (void)fps;
        if (bufSize > 0) buf[0] = L'\0';
    }

    // ================================================================
    // 미리보기
    // ================================================================

    // 제어판 미리보기 창용 콘텐츠 생성
    // nullptr 반환 시 미리보기 창에 아무 것도 표시되지 않음
    // width, height: 미리보기 창 크기
    // forceCPU: true이면 GPU 사용 불가
    virtual std::unique_ptr<IPreviewContent> CreatePreviewContent(
        int width, int height, bool forceCPU) {
        (void)width; (void)height; (void)forceCPU;
        return nullptr;
    }
};

} // namespace wsse
