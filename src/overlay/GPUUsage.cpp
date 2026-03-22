#include "overlay/GPUUsage.h"
#include <pdh.h>
#include <cstdio>
#include <cstdlib>

#pragma comment(lib, "pdh.lib")

namespace wsse {

GPUUsage::~GPUUsage() {
    if (query_) {
        PdhCloseQuery(static_cast<PDH_HQUERY>(query_));
        query_ = nullptr;
    }
    if (counters_) {
        free(counters_);
        counters_ = nullptr;
    }
}

bool GPUUsage::Init() {
    DWORD pid = GetCurrentProcessId();

    // PDH 쿼리 생성
    PDH_HQUERY q = nullptr;
    PDH_STATUS status = PdhOpenQueryW(nullptr, 0, &q);
    if (status != ERROR_SUCCESS) return false;
    query_ = q;

    // 현재 프로세스의 GPU 엔진 카운터 패턴
    wchar_t wildcard[256];
    swprintf_s(wildcard, L"\\GPU Engine(pid_%lu_*)\\Utilization Percentage", pid);

    // 와일드카드 확장
    DWORD bufSize = 0;
    status = PdhExpandWildCardPathW(nullptr, wildcard, nullptr, &bufSize, 0);
    // PDH_MORE_DATA = 0x800007D2
    if (status != static_cast<PDH_STATUS>(0x800007D2L) || bufSize == 0) {
        PdhCloseQuery(q);
        query_ = nullptr;
        return false;
    }

    wchar_t* expandedBuf = static_cast<wchar_t*>(malloc(bufSize * sizeof(wchar_t)));
    if (!expandedBuf) {
        PdhCloseQuery(q);
        query_ = nullptr;
        return false;
    }

    status = PdhExpandWildCardPathW(nullptr, wildcard, expandedBuf, &bufSize, 0);
    if (status != ERROR_SUCCESS) {
        free(expandedBuf);
        PdhCloseQuery(q);
        query_ = nullptr;
        return false;
    }

    // 카운터 수 세기
    int count = 0;
    for (const wchar_t* p = expandedBuf; *p; p += wcslen(p) + 1) {
        count++;
    }

    if (count == 0) {
        free(expandedBuf);
        PdhCloseQuery(q);
        query_ = nullptr;
        return false;
    }

    // 카운터 배열 할당 및 등록
    counters_ = static_cast<void**>(malloc(count * sizeof(void*)));
    if (!counters_) {
        free(expandedBuf);
        PdhCloseQuery(q);
        query_ = nullptr;
        return false;
    }

    counterCount_ = 0;
    for (const wchar_t* p = expandedBuf; *p; p += wcslen(p) + 1) {
        PDH_HCOUNTER counter = nullptr;
        if (PdhAddCounterW(q, p, 0, &counter) == ERROR_SUCCESS) {
            counters_[counterCount_++] = counter;
        }
    }

    free(expandedBuf);

    if (counterCount_ == 0) {
        free(counters_);
        counters_ = nullptr;
        PdhCloseQuery(q);
        query_ = nullptr;
        return false;
    }

    // 첫 번째 샘플 수집 (기준점)
    PdhCollectQueryData(q);
    initialized_ = true;
    return true;
}

void GPUUsage::Reset() {
    if (query_) {
        PdhCloseQuery(static_cast<PDH_HQUERY>(query_));
        query_ = nullptr;
    }
    if (counters_) {
        free(counters_);
        counters_ = nullptr;
    }
    counterCount_ = 0;
    usagePercent_ = 0;
    initialized_ = false;
}

void GPUUsage::Update() {
    // GPU 카운터는 프로세스가 GPU를 사용한 후에야 등록됨
    // 초기화 실패 시 매 Update에서 재시도
    if (!initialized_) return;

    PDH_HQUERY q = static_cast<PDH_HQUERY>(query_);
    PDH_STATUS status = PdhCollectQueryData(q);
    if (status != ERROR_SUCCESS) return;

    // 모든 GPU 엔진의 사용율 합산
    double total = 0.0;
    for (int i = 0; i < counterCount_; i++) {
        PDH_FMT_COUNTERVALUE value{};
        status = PdhGetFormattedCounterValue(
            static_cast<PDH_HCOUNTER>(counters_[i]),
            PDH_FMT_DOUBLE, nullptr, &value);
        if (status == ERROR_SUCCESS) {
            total += value.doubleValue;
        }
    }

    int pct = static_cast<int>(total + 0.5);
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    usagePercent_ = pct;
}

} // namespace wsse
