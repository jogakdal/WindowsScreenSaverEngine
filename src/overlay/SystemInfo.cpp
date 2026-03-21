#include "overlay/SystemInfo.h"
#include "framework/Localization.h"
#include <dxgi.h>
#include <shlwapi.h>

namespace wsse {

// 문자열 앞뒤 공백 제거
static void TrimWhitespace(wchar_t* str) {
    wchar_t* start = str;
    while (*start == L' ' || *start == L'\t') start++;
    if (start != str) {
        size_t len = wcslen(start);
        memmove(str, start, (len + 1) * sizeof(wchar_t));
    }
    size_t len = wcslen(str);
    while (len > 0 && (str[len - 1] == L' ' || str[len - 1] == L'\t'))
        str[--len] = L'\0';
}

// 레지스트리에서 CPU 이름 조회
static void QueryCPUName(wchar_t* buf, size_t bufSize) {
    buf[0] = L'\0';
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD size = static_cast<DWORD>(bufSize * sizeof(wchar_t));
        RegQueryValueExW(hKey, L"ProcessorNameString", nullptr, nullptr,
                         reinterpret_cast<BYTE*>(buf), &size);
        RegCloseKey(hKey);
    }
    TrimWhitespace(buf);
    if (buf[0] == L'\0') wcscpy_s(buf, bufSize, TR(Str::UnknownCPU));
}

// DXGI를 통해 GPU 이름 조회
static void QueryGPUName(wchar_t* buf, size_t bufSize) {
    buf[0] = L'\0';
    IDXGIFactory* factory = nullptr;
    if (SUCCEEDED(CreateDXGIFactory(__uuidof(IDXGIFactory),
                                    reinterpret_cast<void**>(&factory)))) {
        IDXGIAdapter* adapter = nullptr;
        if (SUCCEEDED(factory->EnumAdapters(0, &adapter))) {
            DXGI_ADAPTER_DESC desc;
            if (SUCCEEDED(adapter->GetDesc(&desc)))
                wcsncpy_s(buf, bufSize, desc.Description, _TRUNCATE);
            adapter->Release();
        }
        factory->Release();
    }
    if (buf[0] == L'\0') wcscpy_s(buf, bufSize, TR(Str::UnknownGPU));
}

// 물리 메모리 용량 (GB 단위, 반올림)
static int QueryRAMGB() {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(memInfo);
    if (GlobalMemoryStatusEx(&memInfo))
        return static_cast<int>((memInfo.ullTotalPhys + (1ULL << 29)) >> 30);
    return 0;
}

static void RemoveWord(wchar_t* str, const wchar_t* word) {
    size_t wordLen = wcslen(word);
    wchar_t* pos = str;
    while ((pos = StrStrIW(pos, word)) != nullptr) {
        size_t removeLen = wordLen;
        if (pos[removeLen] == L' ')
            removeLen++;
        memmove(pos, pos + removeLen,
                (wcslen(pos + removeLen) + 1) * sizeof(wchar_t));
    }
    TrimWhitespace(str);
}

static void RemoveTrademarks(wchar_t* str) {
    const wchar_t* marks[] = {
        L"(R)", L"(TM)", L"(C)", L"\u00AE", L"\u2122"
    };
    for (const auto* m : marks)
        RemoveWord(str, m);
}

static void ShortenCPUName(wchar_t* buf) {
    RemoveTrademarks(buf);
    RemoveWord(buf, L"Processor");
    RemoveWord(buf, L"CPU");
    RemoveWord(buf, L"with Radeon Graphics");
    RemoveWord(buf, L"with Radeon Vega Graphics");
    // 연속 공백 정리
    wchar_t* dst = buf;
    bool prevSpace = false;
    for (wchar_t* src = buf; *src; src++) {
        if (*src == L' ') {
            if (!prevSpace)
                *dst++ = L' ';
            prevSpace = true;
        } else {
            *dst++ = *src;
            prevSpace = false;
        }
    }
    *dst = L'\0';
    TrimWhitespace(buf);
}

static void ShortenGPUName(wchar_t* buf) {
    RemoveTrademarks(buf);
    RemoveWord(buf, L"NVIDIA");
    RemoveWord(buf, L"AMD");
    RemoveWord(buf, L"Intel");
    RemoveWord(buf, L"Microsoft");
    RemoveWord(buf, L"Graphics");
    // 연속 공백 정리
    wchar_t* dst = buf;
    bool prevSpace = false;
    for (wchar_t* src = buf; *src; src++) {
        if (*src == L' ') {
            if (!prevSpace)
                *dst++ = L' ';
            prevSpace = true;
        } else {
            *dst++ = *src;
            prevSpace = false;
        }
    }
    *dst = L'\0';
    TrimWhitespace(buf);
}

void SystemInfo::Collect() {
    QueryGPUName(gpuName_, 256);
    QueryCPUName(cpuName_, 256);
    ramGB_ = QueryRAMGB();
    ShortenCPUName(cpuName_);
    ShortenGPUName(gpuName_);
}

} // namespace wsse
