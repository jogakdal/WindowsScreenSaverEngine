#include "framework/Localization.h"
#include <cwchar>

namespace wsse {

Lang Localization::lang_ = Lang::EN;

static constexpr int kLangCount = static_cast<int>(Lang::COUNT);
static constexpr int kStrCount = static_cast<int>(Str::COUNT);

// ---- 프레임워크 번역 테이블 [언어][문자열ID] ----

static const wchar_t* const table[kLangCount][kStrCount] = {

// ============================================================
// EN - English (default)
// ============================================================
{
    // Installer
    L"Administrator privileges are required.",
    L"Failed to install to System32 (error %lu).\nPlease try again with administrator privileges.",
    L"%s has been installed!\n\nClick OK to open Screen Saver settings.",
    L"Uninstall %s?",
    L"%s has been uninstalled.\nSome files are in use and will be deleted after reboot.",
    L"%s has been uninstalled.",
    L"%s v%s is installed.\nReplace with %s?",
    L"Replace\nRemove %s and install %s",
    L"Version v%s is already installed.\nReinstall?",
    L"Reinstall\nReinstall the same version",
    L"Version v%s is already installed.\nUpgrade?",
    L"Upgrade\nUpgrade to the new version",
    L"Version v%s is already installed.\nThe installed version is newer. Downgrade?",
    L"Downgrade\nRevert to the older version",
    L"Choose an action.",
    L"Install\nInstall as screen saver (admin required)",
    L"Preview\nRun without installing",
    L"Uninstall\nRemove the installed screen saver",

    // Settings dialog (framework controls)
    L"%s Settings",
    L"Rendering",
    L"Force CPU rendering (no GPU)",
    L"Display",
    L"Show overlay (zoom / FPS)",
    L"Show clock",
    L"Update",
    L"Check for updates",
    L"OK",
    L"Cancel",

    // Update
    L"Checking for updates...",
    L"New version v%s available!",
    L"Download",
    L"You have the latest version.",
    L"Update check failed.",

    // Overlay
    L"Press F1 for settings",

    // System info
    L"Unknown CPU",
    L"Unknown GPU",
},

// ============================================================
// KO - Korean
// ============================================================
{
    L"\xAD00\xB9AC\xC790 \xAD8C\xD55C\xC774 \xD544\xC694\xD569\xB2C8\xB2E4.",
    L"System32\xC5D0 \xC124\xCE58 \xC2E4\xD328 (\xC624\xB958 %lu).\n\xAD00\xB9AC\xC790 \xAD8C\xD55C\xC73C\xB85C \xB2E4\xC2DC \xC2DC\xB3C4\xD574 \xC8FC\xC138\xC694.",
    L"%s\xAC00 \xC124\xCE58\xB418\xC5C8\xC2B5\xB2C8\xB2E4!\n\n\xD655\xC778\xC744 \xB204\xB974\xBA74 \xD654\xBA74 \xBCF4\xD638\xAE30 \xC124\xC815\xC744 \xC5FD\xB2C8\xB2E4.",
    L"%s\xC744(\xB97C) \xC81C\xAC70\xD558\xC2DC\xACA0\xC2B5\xB2C8\xAE4C?",
    L"%s\xAC00 \xC81C\xAC70\xB418\xC5C8\xC2B5\xB2C8\xB2E4.\n\xD30C\xC77C\xC774 \xC0AC\xC6A9 \xC911\xC774\xC5B4\xC11C \xC7AC\xBD80\xD305 \xD6C4 \xC644\xC804\xD788 \xC0AD\xC81C\xB429\xB2C8\xB2E4.",
    L"%s\xAC00 \xC81C\xAC70\xB418\xC5C8\xC2B5\xB2C8\xB2E4.",
    L"%s v%s\xC774(\xAC00) \xC124\xCE58\xB418\xC5B4 \xC788\xC2B5\xB2C8\xB2E4.\n%s(\xC73C)\xB85C \xAD50\xCCB4\xD558\xC2DC\xACA0\xC2B5\xB2C8\xAE4C?",
    L"\xAD50\xCCB4\n%s\xC744(\xB97C) \xC81C\xAC70\xD558\xACE0 %s\xC744(\xB97C) \xC124\xCE58\xD569\xB2C8\xB2E4",
    L"\xC774\xBBF8 \xC124\xCE58\xB41C \xBC84\xC804\xC774 \xC788\xC2B5\xB2C8\xB2E4: v%s\n\xC7AC\xC124\xCE58\xD558\xC2DC\xACA0\xC2B5\xB2C8\xAE4C?",
    L"\xC7AC\xC124\xCE58\n\xB3D9\xC77C\xD55C \xBC84\xC804\xC744 \xB2E4\xC2DC \xC124\xCE58\xD569\xB2C8\xB2E4",
    L"\xC774\xBBF8 \xC124\xCE58\xB41C \xBC84\xC804\xC774 \xC788\xC2B5\xB2C8\xB2E4: v%s\n\xC5C5\xADF8\xB808\xC774\xB4DC\xD558\xC2DC\xACA0\xC2B5\xB2C8\xAE4C?",
    L"\xC5C5\xADF8\xB808\xC774\xB4DC\n\xC0C8 \xBC84\xC804\xC73C\xB85C \xC5C5\xADF8\xB808\xC774\xB4DC\xD569\xB2C8\xB2E4",
    L"\xC774\xBBF8 \xC124\xCE58\xB41C \xBC84\xC804\xC774 \xC788\xC2B5\xB2C8\xB2E4: v%s\n\xC124\xCE58\xB41C \xBC84\xC804\xC774 \xC0C1\xC704 \xBC84\xC804\xC785\xB2C8\xB2E4. \xB2E4\xC6B4\xADF8\xB808\xC774\xB4DC\xD558\xC2DC\xACA0\xC2B5\xB2C8\xAE4C?",
    L"\xB2E4\xC6B4\xADF8\xB808\xC774\xB4DC\n\xC774\xC804 \xBC84\xC804\xC73C\xB85C \xB418\xB3CC\xB9BD\xB2C8\xB2E4",
    L"\xC6D0\xD558\xB294 \xC791\xC5C5\xC744 \xC120\xD0DD\xD558\xC138\xC694.",
    L"\xC124\xCE58\n\xD654\xBA74 \xBCF4\xD638\xAE30\xB85C \xC124\xCE58\xD569\xB2C8\xB2E4 (\xAD00\xB9AC\xC790 \xAD8C\xD55C \xD544\xC694)",
    L"\xBBF8\xB9AC\xBCF4\xAE30\n\xC124\xCE58 \xC5C6\xC774 \xBC14\xB85C \xC2E4\xD589\xD569\xB2C8\xB2E4",
    L"\xC81C\xAC70\n\xC124\xCE58\xB41C \xD654\xBA74 \xBCF4\xD638\xAE30\xB97C \xC81C\xAC70\xD569\xB2C8\xB2E4",

    // Settings
    L"%s \xC124\xC815",
    L"\xB80C\xB354\xB9C1",
    L"CPU \xAC15\xC81C \xB80C\xB354\xB9C1 (GPU \xBBF8\xC0AC\xC6A9)",
    L"\xB514\xC2A4\xD50C\xB808\xC774",
    L"\xC624\xBC84\xB808\xC774 \xD45C\xC2DC (\xC90C / FPS)",
    L"\xC2DC\xACC4 \xD45C\xC2DC",
    L"\xC5C5\xB370\xC774\xD2B8",
    L"\xC5C5\xB370\xC774\xD2B8 \xD655\xC778",
    L"\xD655\xC778",
    L"\xCDE8\xC18C",

    // Update
    L"\xC5C5\xB370\xC774\xD2B8 \xD655\xC778 \xC911...",
    L"\xC0C8 \xBC84\xC804 v%s \xC0AC\xC6A9 \xAC00\xB2A5!",
    L"\xB2E4\xC6B4\xB85C\xB4DC",
    L"\xCD5C\xC2E0 \xBC84\xC804\xC785\xB2C8\xB2E4.",
    L"\xC5C5\xB370\xC774\xD2B8 \xD655\xC778 \xC2E4\xD328.",

    // Overlay
    L"F1 \xD0A4\xB97C \xB204\xB974\xBA74 \xC124\xC815",

    // System
    L"\xC54C \xC218 \xC5C6\xB294 CPU",
    L"\xC54C \xC218 \xC5C6\xB294 GPU",
},

// ============================================================
// JA - Japanese
// ============================================================
{
    L"\u7BA1\u7406\u8005\u6A29\u9650\u304C\u5FC5\u8981\u3067\u3059\u3002",
    L"System32\u3078\u306E\u30A4\u30F3\u30B9\u30C8\u30FC\u30EB\u306B\u5931\u6557\u3057\u307E\u3057\u305F (\u30A8\u30E9\u30FC %lu)\u3002\n\u7BA1\u7406\u8005\u6A29\u9650\u3067\u518D\u8A66\u884C\u3057\u3066\u304F\u3060\u3055\u3044\u3002",
    L"%s \u304C\u30A4\u30F3\u30B9\u30C8\u30FC\u30EB\u3055\u308C\u307E\u3057\u305F\uFF01\n\nOK\u3092\u30AF\u30EA\u30C3\u30AF\u3057\u3066\u30B9\u30AF\u30EA\u30FC\u30F3\u30BB\u30FC\u30D0\u30FC\u8A2D\u5B9A\u3092\u958B\u304D\u307E\u3059\u3002",
    L"%s \u3092\u30A2\u30F3\u30A4\u30F3\u30B9\u30C8\u30FC\u30EB\u3057\u307E\u3059\u304B\uFF1F",
    L"%s \u304C\u30A2\u30F3\u30A4\u30F3\u30B9\u30C8\u30FC\u30EB\u3055\u308C\u307E\u3057\u305F\u3002\n\u4F7F\u7528\u4E2D\u306E\u30D5\u30A1\u30A4\u30EB\u306F\u518D\u8D77\u52D5\u5F8C\u306B\u524A\u9664\u3055\u308C\u307E\u3059\u3002",
    L"%s \u304C\u30A2\u30F3\u30A4\u30F3\u30B9\u30C8\u30FC\u30EB\u3055\u308C\u307E\u3057\u305F\u3002",
    L"%s v%s \u304C\u30A4\u30F3\u30B9\u30C8\u30FC\u30EB\u3055\u308C\u3066\u3044\u307E\u3059\u3002\n%s \u306B\u7F6E\u304D\u63DB\u3048\u307E\u3059\u304B\uFF1F",
    L"\u7F6E\u304D\u63DB\u3048\n%s \u3092\u524A\u9664\u3057\u3066 %s \u3092\u30A4\u30F3\u30B9\u30C8\u30FC\u30EB\u3057\u307E\u3059",
    L"\u30D0\u30FC\u30B8\u30E7\u30F3 v%s \u304C\u65E2\u306B\u30A4\u30F3\u30B9\u30C8\u30FC\u30EB\u3055\u308C\u3066\u3044\u307E\u3059\u3002\n\u518D\u30A4\u30F3\u30B9\u30C8\u30FC\u30EB\u3057\u307E\u3059\u304B\uFF1F",
    L"\u518D\u30A4\u30F3\u30B9\u30C8\u30FC\u30EB\n\u540C\u3058\u30D0\u30FC\u30B8\u30E7\u30F3\u3092\u518D\u30A4\u30F3\u30B9\u30C8\u30FC\u30EB\u3057\u307E\u3059",
    L"\u30D0\u30FC\u30B8\u30E7\u30F3 v%s \u304C\u65E2\u306B\u30A4\u30F3\u30B9\u30C8\u30FC\u30EB\u3055\u308C\u3066\u3044\u307E\u3059\u3002\n\u30A2\u30C3\u30D7\u30B0\u30EC\u30FC\u30C9\u3057\u307E\u3059\u304B\uFF1F",
    L"\u30A2\u30C3\u30D7\u30B0\u30EC\u30FC\u30C9\n\u65B0\u3057\u3044\u30D0\u30FC\u30B8\u30E7\u30F3\u306B\u30A2\u30C3\u30D7\u30B0\u30EC\u30FC\u30C9\u3057\u307E\u3059",
    L"\u30D0\u30FC\u30B8\u30E7\u30F3 v%s \u304C\u65E2\u306B\u30A4\u30F3\u30B9\u30C8\u30FC\u30EB\u3055\u308C\u3066\u3044\u307E\u3059\u3002\n\u30A4\u30F3\u30B9\u30C8\u30FC\u30EB\u6E08\u307F\u306E\u65B9\u304C\u65B0\u3057\u3044\u3067\u3059\u3002\u30C0\u30A6\u30F3\u30B0\u30EC\u30FC\u30C9\u3057\u307E\u3059\u304B\uFF1F",
    L"\u30C0\u30A6\u30F3\u30B0\u30EC\u30FC\u30C9\n\u4EE5\u524D\u306E\u30D0\u30FC\u30B8\u30E7\u30F3\u306B\u623B\u3057\u307E\u3059",
    L"\u64CD\u4F5C\u3092\u9078\u629E\u3057\u3066\u304F\u3060\u3055\u3044\u3002",
    L"\u30A4\u30F3\u30B9\u30C8\u30FC\u30EB\n\u30B9\u30AF\u30EA\u30FC\u30F3\u30BB\u30FC\u30D0\u30FC\u3068\u3057\u3066\u30A4\u30F3\u30B9\u30C8\u30FC\u30EB (\u7BA1\u7406\u8005\u6A29\u9650\u304C\u5FC5\u8981)",
    L"\u30D7\u30EC\u30D3\u30E5\u30FC\n\u30A4\u30F3\u30B9\u30C8\u30FC\u30EB\u305B\u305A\u306B\u5B9F\u884C\u3057\u307E\u3059",
    L"\u30A2\u30F3\u30A4\u30F3\u30B9\u30C8\u30FC\u30EB\n\u30A4\u30F3\u30B9\u30C8\u30FC\u30EB\u6E08\u307F\u306E\u30B9\u30AF\u30EA\u30FC\u30F3\u30BB\u30FC\u30D0\u30FC\u3092\u524A\u9664\u3057\u307E\u3059",

    // Settings
    L"%s \u8A2D\u5B9A",
    L"\u30EC\u30F3\u30C0\u30EA\u30F3\u30B0",
    L"CPU\u30EC\u30F3\u30C0\u30EA\u30F3\u30B0\u3092\u5F37\u5236 (GPU\u306A\u3057)",
    L"\u8868\u793A",
    L"\u30AA\u30FC\u30D0\u30FC\u30EC\u30A4\u8868\u793A (\u30BA\u30FC\u30E0 / FPS)",
    L"\u6642\u8A08\u8868\u793A",
    L"\u30A2\u30C3\u30D7\u30C7\u30FC\u30C8",
    L"\u30A2\u30C3\u30D7\u30C7\u30FC\u30C8\u78BA\u8A8D",
    L"OK",
    L"\u30AD\u30E3\u30F3\u30BB\u30EB",

    // Update
    L"\u30A2\u30C3\u30D7\u30C7\u30FC\u30C8\u3092\u78BA\u8A8D\u4E2D...",
    L"\u65B0\u30D0\u30FC\u30B8\u30E7\u30F3 v%s \u304C\u5229\u7528\u53EF\u80FD\uFF01",
    L"\u30C0\u30A6\u30F3\u30ED\u30FC\u30C9",
    L"\u6700\u65B0\u30D0\u30FC\u30B8\u30E7\u30F3\u3067\u3059\u3002",
    L"\u30A2\u30C3\u30D7\u30C7\u30FC\u30C8\u78BA\u8A8D\u306B\u5931\u6557\u3057\u307E\u3057\u305F\u3002",

    // Overlay
    L"F1\u30AD\u30FC\u3067\u8A2D\u5B9A",

    // System
    L"\u4E0D\u660E\u306ACPU",
    L"\u4E0D\u660E\u306AGPU",
},

// ============================================================
// ZH - Chinese (Simplified)
// ============================================================
{
    L"\u9700\u8981\u7BA1\u7406\u5458\u6743\u9650\u3002",
    L"\u5B89\u88C5\u5230 System32 \u5931\u8D25 (\u9519\u8BEF %lu)\u3002\n\u8BF7\u4EE5\u7BA1\u7406\u5458\u6743\u9650\u91CD\u8BD5\u3002",
    L"%s \u5DF2\u5B89\u88C5\uFF01\n\n\u70B9\u51FB\u786E\u5B9A\u6253\u5F00\u5C4F\u5E55\u4FDD\u62A4\u7A0B\u5E8F\u8BBE\u7F6E\u3002",
    L"\u5378\u8F7D %s\uFF1F",
    L"%s \u5DF2\u5378\u8F7D\u3002\n\u90E8\u5206\u6587\u4EF6\u6B63\u5728\u4F7F\u7528\u4E2D\uFF0C\u5C06\u5728\u91CD\u542F\u540E\u5220\u9664\u3002",
    L"%s \u5DF2\u5378\u8F7D\u3002",
    L"%s v%s \u5DF2\u5B89\u88C5\u3002\n\u66FF\u6362\u4E3A %s\uFF1F",
    L"\u66FF\u6362\n\u5220\u9664 %s \u5E76\u5B89\u88C5 %s",
    L"\u5DF2\u5B89\u88C5\u7248\u672C v%s\u3002\n\u91CD\u65B0\u5B89\u88C5\uFF1F",
    L"\u91CD\u65B0\u5B89\u88C5\n\u91CD\u65B0\u5B89\u88C5\u76F8\u540C\u7248\u672C",
    L"\u5DF2\u5B89\u88C5\u7248\u672C v%s\u3002\n\u5347\u7EA7\uFF1F",
    L"\u5347\u7EA7\n\u5347\u7EA7\u5230\u65B0\u7248\u672C",
    L"\u5DF2\u5B89\u88C5\u7248\u672C v%s\u3002\n\u5DF2\u5B89\u88C5\u7684\u7248\u672C\u66F4\u65B0\u3002\u964D\u7EA7\uFF1F",
    L"\u964D\u7EA7\n\u6062\u590D\u5230\u65E7\u7248\u672C",
    L"\u8BF7\u9009\u62E9\u64CD\u4F5C\u3002",
    L"\u5B89\u88C5\n\u5B89\u88C5\u4E3A\u5C4F\u5E55\u4FDD\u62A4\u7A0B\u5E8F (\u9700\u8981\u7BA1\u7406\u5458\u6743\u9650)",
    L"\u9884\u89C8\n\u65E0\u9700\u5B89\u88C5\u76F4\u63A5\u8FD0\u884C",
    L"\u5378\u8F7D\n\u5220\u9664\u5DF2\u5B89\u88C5\u7684\u5C4F\u5E55\u4FDD\u62A4\u7A0B\u5E8F",

    // Settings
    L"%s \u8BBE\u7F6E",
    L"\u6E32\u67D3",
    L"\u5F3A\u5236 CPU \u6E32\u67D3 (\u4E0D\u4F7F\u7528 GPU)",
    L"\u663E\u793A",
    L"\u663E\u793A\u53E0\u52A0\u4FE1\u606F (\u7F29\u653E / FPS)",
    L"\u663E\u793A\u65F6\u949F",
    L"\u66F4\u65B0",
    L"\u68C0\u67E5\u66F4\u65B0",
    L"\u786E\u5B9A",
    L"\u53D6\u6D88",

    // Update
    L"\u6B63\u5728\u68C0\u67E5\u66F4\u65B0...",
    L"\u65B0\u7248\u672C v%s \u53EF\u7528\uFF01",
    L"\u4E0B\u8F7D",
    L"\u5DF2\u662F\u6700\u65B0\u7248\u672C\u3002",
    L"\u66F4\u65B0\u68C0\u67E5\u5931\u8D25\u3002",

    // Overlay
    L"\u6309 F1 \u6253\u5F00\u8BBE\u7F6E",

    // System
    L"\u672A\u77E5 CPU",
    L"\u672A\u77E5 GPU",
},

// ============================================================
// DE - German
// ============================================================
{
    L"Administratorrechte erforderlich.",
    L"Installation in System32 fehlgeschlagen (Fehler %lu).\nBitte mit Administratorrechten erneut versuchen.",
    L"%s wurde installiert!\n\nKlicken Sie OK, um die Bildschirmschoner-Einstellungen zu \u00F6ffnen.",
    L"%s deinstallieren?",
    L"%s wurde deinstalliert.\nEinige Dateien werden nach dem Neustart gel\u00F6scht.",
    L"%s wurde deinstalliert.",
    L"%s v%s ist installiert.\nDurch %s ersetzen?",
    L"Ersetzen\n%s entfernen und %s installieren",
    L"Version v%s ist bereits installiert.\nNeu installieren?",
    L"Neu installieren\nDieselbe Version erneut installieren",
    L"Version v%s ist bereits installiert.\nUpgrade durchf\u00FChren?",
    L"Upgrade\nAuf die neue Version aktualisieren",
    L"Version v%s ist bereits installiert.\nDie installierte Version ist neuer. Downgrade durchf\u00FChren?",
    L"Downgrade\nZur \u00E4lteren Version zur\u00FCckkehren",
    L"W\u00E4hlen Sie eine Aktion.",
    L"Installieren\nAls Bildschirmschoner installieren (Admin erforderlich)",
    L"Vorschau\nOhne Installation ausf\u00FChren",
    L"Deinstallieren\nInstallierten Bildschirmschoner entfernen",

    // Settings
    L"%s Einstellungen",
    L"Rendering",
    L"CPU-Rendering erzwingen (kein GPU)",
    L"Anzeige",
    L"Overlay anzeigen (Zoom / FPS)",
    L"Uhr anzeigen",
    L"Update",
    L"Nach Updates suchen",
    L"OK",
    L"Abbrechen",

    // Update
    L"Suche nach Updates...",
    L"Neue Version v%s verf\u00FCgbar!",
    L"Herunterladen",
    L"Sie haben die neueste Version.",
    L"Update-Pr\u00FCfung fehlgeschlagen.",

    // Overlay
    L"F1 f\u00FCr Einstellungen",

    // System
    L"Unbekannte CPU",
    L"Unbekannte GPU",
},

// ============================================================
// FR - French
// ============================================================
{
    L"Privil\u00E8ges administrateur requis.",
    L"\u00C9chec de l'installation dans System32 (erreur %lu).\nVeuillez r\u00E9essayer avec les droits administrateur.",
    L"%s a \u00E9t\u00E9 install\u00E9 !\n\nCliquez OK pour ouvrir les param\u00E8tres de l'\u00E9cran de veille.",
    L"D\u00E9sinstaller %s ?",
    L"%s a \u00E9t\u00E9 d\u00E9sinstall\u00E9.\nCertains fichiers seront supprim\u00E9s apr\u00E8s le red\u00E9marrage.",
    L"%s a \u00E9t\u00E9 d\u00E9sinstall\u00E9.",
    L"%s v%s est install\u00E9.\nRemplacer par %s ?",
    L"Remplacer\nSupprimer %s et installer %s",
    L"La version v%s est d\u00E9j\u00E0 install\u00E9e.\nR\u00E9installer ?",
    L"R\u00E9installer\nR\u00E9installer la m\u00EAme version",
    L"La version v%s est d\u00E9j\u00E0 install\u00E9e.\nMettre \u00E0 jour ?",
    L"Mise \u00E0 jour\nMettre \u00E0 jour vers la nouvelle version",
    L"La version v%s est d\u00E9j\u00E0 install\u00E9e.\nLa version install\u00E9e est plus r\u00E9cente. R\u00E9trograder ?",
    L"R\u00E9trogradation\nRevenir \u00E0 l'ancienne version",
    L"Choisissez une action.",
    L"Installer\nInstaller comme \u00E9cran de veille (admin requis)",
    L"Aper\u00E7u\nEx\u00E9cuter sans installer",
    L"D\u00E9sinstaller\nSupprimer l'\u00E9cran de veille install\u00E9",

    // Settings
    L"Param\u00E8tres de %s",
    L"Rendu",
    L"Forcer le rendu CPU (pas de GPU)",
    L"Affichage",
    L"Afficher l'overlay (zoom / FPS)",
    L"Afficher l'horloge",
    L"Mise \u00E0 jour",
    L"V\u00E9rifier les mises \u00E0 jour",
    L"OK",
    L"Annuler",

    // Update
    L"V\u00E9rification des mises \u00E0 jour...",
    L"Nouvelle version v%s disponible !",
    L"T\u00E9l\u00E9charger",
    L"Vous avez la derni\u00E8re version.",
    L"\u00C9chec de la v\u00E9rification.",

    // Overlay
    L"F1 pour les param\u00E8tres",

    // System
    L"CPU inconnu",
    L"GPU inconnu",
},

// ============================================================
// ES - Spanish
// ============================================================
{
    L"Se requieren privilegios de administrador.",
    L"Error al instalar en System32 (error %lu).\nInt\u00E9ntelo de nuevo con privilegios de administrador.",
    L"\u00A1%s se ha instalado!\n\nHaga clic en Aceptar para abrir la configuraci\u00F3n del protector de pantalla.",
    L"\u00BFDesinstalar %s?",
    L"%s se ha desinstalado.\nAlgunos archivos se eliminar\u00E1n despu\u00E9s de reiniciar.",
    L"%s se ha desinstalado.",
    L"%s v%s est\u00E1 instalado.\n\u00BFReemplazar con %s?",
    L"Reemplazar\nEliminar %s e instalar %s",
    L"La versi\u00F3n v%s ya est\u00E1 instalada.\n\u00BFReinstalar?",
    L"Reinstalar\nReinstalar la misma versi\u00F3n",
    L"La versi\u00F3n v%s ya est\u00E1 instalada.\n\u00BFActualizar?",
    L"Actualizar\nActualizar a la nueva versi\u00F3n",
    L"La versi\u00F3n v%s ya est\u00E1 instalada.\nLa versi\u00F3n instalada es m\u00E1s reciente. \u00BFDegregar?",
    L"Degradar\nVolver a la versi\u00F3n anterior",
    L"Elija una acci\u00F3n.",
    L"Instalar\nInstalar como protector de pantalla (requiere admin)",
    L"Vista previa\nEjecutar sin instalar",
    L"Desinstalar\nEliminar el protector de pantalla instalado",

    // Settings
    L"Configuraci\u00F3n de %s",
    L"Renderizado",
    L"Forzar renderizado CPU (sin GPU)",
    L"Pantalla",
    L"Mostrar overlay (zoom / FPS)",
    L"Mostrar reloj",
    L"Actualizaci\u00F3n",
    L"Buscar actualizaciones",
    L"Aceptar",
    L"Cancelar",

    // Update
    L"Buscando actualizaciones...",
    L"\u00A1Nueva versi\u00F3n v%s disponible!",
    L"Descargar",
    L"Tiene la \u00FAltima versi\u00F3n.",
    L"Error al buscar actualizaciones.",

    // Overlay
    L"F1 para configuraci\u00F3n",

    // System
    L"CPU desconocida",
    L"GPU desconocida",
},

}; // end table

// ---- 구현 ----

Lang Localization::DetectLang() {
    LANGID lid = GetUserDefaultUILanguage();
    switch (PRIMARYLANGID(lid)) {
    case LANG_KOREAN:     return Lang::KO;
    case LANG_JAPANESE:   return Lang::JA;
    case LANG_CHINESE:    return Lang::ZH;
    case LANG_GERMAN:     return Lang::DE;
    case LANG_FRENCH:     return Lang::FR;
    case LANG_SPANISH:    return Lang::ES;
    default:              return Lang::EN;
    }
}

Lang Localization::ParseCode(const wchar_t* code) {
    if (!code) return Lang::EN;
    if (_wcsnicmp(code, L"ko", 2) == 0) return Lang::KO;
    if (_wcsnicmp(code, L"ja", 2) == 0) return Lang::JA;
    if (_wcsnicmp(code, L"zh", 2) == 0) return Lang::ZH;
    if (_wcsnicmp(code, L"de", 2) == 0) return Lang::DE;
    if (_wcsnicmp(code, L"fr", 2) == 0) return Lang::FR;
    if (_wcsnicmp(code, L"es", 2) == 0) return Lang::ES;
    return Lang::EN;
}

void Localization::Init(const wchar_t* langOverride) {
    lang_ = langOverride ? ParseCode(langOverride) : DetectLang();
}

const wchar_t* Localization::Get(Str id) {
    int langIdx = static_cast<int>(lang_);
    int strIdx = static_cast<int>(id);
    if (langIdx < 0 || langIdx >= kLangCount) langIdx = 0;
    if (strIdx < 0 || strIdx >= kStrCount) return L"";
    const wchar_t* s = table[langIdx][strIdx];
    return s ? s : table[0][strIdx];
}

} // namespace wsse
