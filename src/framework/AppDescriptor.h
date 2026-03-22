#pragma once
#include <windows.h>

namespace wsse {

// 스크린세이버 앱 메타데이터
//
// 프레임워크가 설치/제거, 자동 업데이트, 레지스트리 관리에 사용하는
// 앱별 상수를 정의합니다. 콘텐츠 구현체가 이 구조체를 채워 엔진에 전달합니다.
struct AppDescriptor {
    // ---- 파일 및 표시 이름 ----

    // System32에 설치되는 .scr 파일명 (e.g., "MyApp.scr")
    const wchar_t* scrName = nullptr;

    // 메시지 박스 제목 등에 쓰이는 짧은 앱 이름 (e.g., "MyApp")
    const wchar_t* appTitle = nullptr;

    // Full <-> Lite 교차 에디션 파일명 (없으면 nullptr)
    const wchar_t* otherScrName = nullptr;
    const wchar_t* otherAppTitle = nullptr;

    // ---- 레지스트리 ----

    // 설정 저장 키 (HKCU 하위, e.g., "Software\\MyApp")
    const wchar_t* registryKey = nullptr;

    // ---- 자동 업데이트 (GitHub Releases API) ----

    // API 호스트 (e.g., "api.github.com")
    const wchar_t* updateHost = nullptr;

    // API 경로 (e.g., "/repos/user/repo/releases/latest")
    const wchar_t* updatePath = nullptr;

    // HTTP User-Agent 헤더 (e.g., "MyApp/1.0")
    const wchar_t* userAgent = nullptr;

    // ---- 기능 플래그 ----
    // 프레임워크 제공 기능 활성화 여부. false이면 해당 기능이
    // 엔진에서 초기화/렌더링되지 않고, 설정 다이얼로그에서도 숨겨집니다.

    // 텍스트 오버레이 (시스템 정보 + 콘텐츠 동적 라인)
    bool hasOverlay = true;

    // 7-세그먼트 디지털 시계
    bool hasClock = true;

    // GPU 렌더링 지원 여부
    // false이면 설정 다이얼로그에서 렌더링 섹션(CPU 강제) 숨김
    bool hasGPU = true;

    // GitHub Releases API 자동 업데이트 확인
    bool hasAutoUpdate = true;

    // ---- 리소스 ----

    // 설정 다이얼로그 리소스 ID (e.g., IDD_CONFIG)
    // 콘텐츠가 자체 RC 파일에서 다이얼로그를 정의하고, 해당 ID를 지정합니다.
    // 프레임워크는 표준 컨트롤 ID(IDC_FORCE_CPU 등)가 있으면 사용하고,
    // 없으면 해당 컨트롤을 건너뜁니다.
    int dialogResourceId = 0;
};

} // namespace wsse
