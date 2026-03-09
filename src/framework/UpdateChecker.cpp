#include "framework/UpdateChecker.h"
#include <winhttp.h>
#include <new>
#include <cstdio>
#include <cstring>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "version.lib")

namespace wsse {

// 업데이트 스레드 파라미터
struct UpdateThreadParams {
    HWND hwnd;
    wchar_t host[256];
    wchar_t path[512];
    wchar_t userAgent[128];
};

// 자신의 FILEVERSION 추출
static bool GetSelfVersion(DWORD& major, DWORD& minor, DWORD& patch) {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);

    DWORD dummy = 0;
    DWORD size = GetFileVersionInfoSizeW(path, &dummy);
    if (size == 0) return false;

    BYTE* buf = new (std::nothrow) BYTE[size];
    if (!buf) return false;

    bool ok = false;
    if (GetFileVersionInfoW(path, 0, size, buf)) {
        VS_FIXEDFILEINFO* ffi = nullptr;
        UINT ffiLen = 0;
        if (VerQueryValueW(buf, L"\\", reinterpret_cast<void**>(&ffi), &ffiLen)) {
            major = HIWORD(ffi->dwFileVersionMS);
            minor = LOWORD(ffi->dwFileVersionMS);
            patch = HIWORD(ffi->dwFileVersionLS);
            ok = true;
        }
    }

    delete[] buf;
    return ok;
}

// "v1.2.0" 또는 "1.2.0" 형태의 태그에서 버전 추출
static bool ParseVersion(const char* tag, DWORD& major, DWORD& minor, DWORD& patch) {
    const char* p = tag;
    if (*p == 'v' || *p == 'V') p++;
    int maj = 0;
    int min = 0;
    int pat = 0;
    if (sscanf_s(p, "%d.%d.%d", &maj, &min, &pat) == 3) {
        major = static_cast<DWORD>(maj);
        minor = static_cast<DWORD>(min);
        patch = static_cast<DWORD>(pat);
        return true;
    }
    return false;
}

// JSON에서 "key":"value" 형태의 문자열 값 추출 (최소한의 파서)
static bool ExtractJsonString(const char* json, const char* key, char* out, int maxLen) {
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char* pos = strstr(json, pattern);
    if (!pos) return false;

    pos += strlen(pattern);
    while (*pos == ' ' || *pos == ':' || *pos == '\t' || *pos == '\n' || *pos == '\r') pos++;
    if (*pos != '"') return false;
    pos++;

    int i = 0;
    while (*pos && *pos != '"' && i < maxLen - 1) {
        // JSON에서 \/ 이스케이프 처리
        if (*pos == '\\' && *(pos + 1) == '/') {
            out[i++] = '/';
            pos += 2;
        } else {
            out[i++] = *pos++;
        }
    }
    out[i] = '\0';
    return i > 0;
}

static DWORD WINAPI UpdateThreadProc(LPVOID param) {
    auto* params = static_cast<UpdateThreadParams*>(param);
    HWND hwnd = params->hwnd;

    HINTERNET hSession = WinHttpOpen(params->userAgent,
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) {
        PostMessageW(hwnd, WM_UPDATE_RESULT, UPDATE_ERROR, 0);
        delete params;
        return 0;
    }

    HINTERNET hConnect = WinHttpConnect(hSession, params->host,
        INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        PostMessageW(hwnd, WM_UPDATE_RESULT, UPDATE_ERROR, 0);
        delete params;
        return 0;
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", params->path,
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        PostMessageW(hwnd, WM_UPDATE_RESULT, UPDATE_ERROR, 0);
        delete params;
        return 0;
    }

    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
            WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !WinHttpReceiveResponse(hRequest, nullptr)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        PostMessageW(hwnd, WM_UPDATE_RESULT, UPDATE_ERROR, 0);
        delete params;
        return 0;
    }

    // 응답 읽기
    char response[8192] = {};
    DWORD totalRead = 0;
    DWORD bytesRead = 0;
    while (WinHttpReadData(hRequest, response + totalRead,
            sizeof(response) - totalRead - 1, &bytesRead) && bytesRead > 0) {
        totalRead += bytesRead;
        if (totalRead >= sizeof(response) - 1) break;
    }
    response[totalRead] = '\0';

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    delete params;

    // tag_name과 html_url 추출
    char tagName[64] = {};
    char htmlUrl[512] = {};
    if (!ExtractJsonString(response, "tag_name", tagName, sizeof(tagName))) {
        PostMessageW(hwnd, WM_UPDATE_RESULT, UPDATE_ERROR, 0);
        return 0;
    }
    ExtractJsonString(response, "html_url", htmlUrl, sizeof(htmlUrl));

    // 원격 버전 파싱
    DWORD remoteMajor = 0;
    DWORD remoteMinor = 0;
    DWORD remotePatch = 0;
    if (!ParseVersion(tagName, remoteMajor, remoteMinor, remotePatch)) {
        PostMessageW(hwnd, WM_UPDATE_RESULT, UPDATE_ERROR, 0);
        return 0;
    }

    // 현재 버전 가져오기
    DWORD selfMajor = 0;
    DWORD selfMinor = 0;
    DWORD selfPatch = 0;
    if (!GetSelfVersion(selfMajor, selfMinor, selfPatch)) {
        PostMessageW(hwnd, WM_UPDATE_RESULT, UPDATE_ERROR, 0);
        return 0;
    }

    // 원격이 더 새로운지 비교
    bool newer = false;
    if (remoteMajor > selfMajor) newer = true;
    else if (remoteMajor == selfMajor && remoteMinor > selfMinor) newer = true;
    else if (remoteMajor == selfMajor && remoteMinor == selfMinor && remotePatch > selfPatch) newer = true;

    if (newer) {
        UpdateInfo* info = new (std::nothrow) UpdateInfo{};
        if (info) {
            swprintf_s(info->version, L"%lu.%lu.%lu", remoteMajor, remoteMinor, remotePatch);
            MultiByteToWideChar(CP_UTF8, 0, htmlUrl, -1, info->url, 512);
        }
        PostMessageW(hwnd, WM_UPDATE_RESULT, UPDATE_AVAILABLE,
            reinterpret_cast<LPARAM>(info));
    } else {
        PostMessageW(hwnd, WM_UPDATE_RESULT, UPDATE_NONE, 0);
    }

    return 0;
}

void CheckForUpdateAsync(HWND hwnd,
                         const wchar_t* host,
                         const wchar_t* path,
                         const wchar_t* userAgent) {
    auto* params = new (std::nothrow) UpdateThreadParams{};
    if (!params) return;

    params->hwnd = hwnd;
    wcscpy_s(params->host, host ? host : L"");
    wcscpy_s(params->path, path ? path : L"");
    wcscpy_s(params->userAgent, userAgent ? userAgent : L"ScreenSaver/1.0");

    HANDLE hThread = CreateThread(nullptr, 0, UpdateThreadProc,
        params, 0, nullptr);
    if (hThread) {
        CloseHandle(hThread);
    } else {
        delete params;
    }
}

} // namespace wsse
