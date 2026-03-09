#pragma once
#include <windows.h>

namespace wsse {

// 미리보기 콘텐츠 인터페이스
//
// 윈도우 제어판의 화면 보호기 미리보기 창(/p 모드)에 표시할 콘텐츠를 정의합니다.
// 메인 콘텐츠(IScreenSaverContent)와 별도로 구현하여 미리보기에 최적화된
// 경량 렌더링을 제공할 수 있습니다.
//
// 사용 예:
//   class MyPreview : public IPreviewContent {
//       void Init(int w, int h, bool) override { /* ... */ }
//       void Update(double dt) override { /* ... */ }
//       void Render() override { /* 동기 렌더링 */ }
//       void BlitTo(HDC hdc, int dw, int dh) override { /* 화면 출력 */ }
//   };
class IPreviewContent {
public:
    virtual ~IPreviewContent() = default;

    // 렌더러 초기화
    // width, height: 미리보기 창 크기 (보통 152x112)
    // forceCPU: true이면 GPU 사용 불가
    virtual void Init(int width, int height, bool forceCPU) = 0;

    // 애니메이션 상태 갱신
    // dt: 이전 프레임으로부터 경과 시간 (초)
    virtual void Update(double dt) = 0;

    // 한 프레임 동기 렌더링 (완료까지 블록)
    virtual void Render() = 0;

    // 렌더링 결과를 대상 DC에 복사
    // destW, destH: 대상 영역 크기 (미리보기 창 크기와 다를 수 있음)
    virtual void BlitTo(HDC hdc, int destW, int destH) = 0;
};

} // namespace wsse
