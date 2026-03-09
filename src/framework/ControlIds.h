#pragma once

// 프레임워크 공통 컨트롤 ID
//
// 설정 다이얼로그에서 프레임워크가 사용하는 표준 컨트롤 ID.
// 앱의 리소스 파일(.rc)에서 다이얼로그를 정의할 때 이 ID를 사용해야 합니다.
// 앱 고유 컨트롤은 이 범위와 겹치지 않는 ID를 사용하세요.
//
// 모든 컨트롤은 선택적입니다. 다이얼로그에 해당 ID의 컨트롤이 없으면
// 프레임워크가 자동으로 건너뜁니다.

// 렌더링 그룹
#define IDC_GRP_RENDERING   1020
#define IDC_FORCE_CPU       1006

// 디스플레이 그룹
#define IDC_GRP_DISPLAY     1022
#define IDC_SHOW_OVERLAY    1008
#define IDC_SHOW_CLOCK      1009

// 업데이트 그룹
#define IDC_GRP_UPDATE      1023
#define IDC_UPDATE_CHECK    1010
#define IDC_UPDATE_STATUS   1011

// 버전 표시
#define IDC_VERSION_LABEL   1012
