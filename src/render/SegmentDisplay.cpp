#include "render/SegmentDisplay.h"

namespace wsse {

// 숫자 세그먼트 패턴: 비트 = 0bGFEDCBA
//  aaa
// f   b
//  ggg
// e   c
//  ddd
static constexpr BYTE kSegDigits[10] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F
};

// 수평 육각형 세그먼트 (중심 cx, cy)
static void DrawHSegment(HDC hdc, int cx, int cy, int halfLen, int ht) {
    POINT pts[6] = {
        { cx - halfLen, cy },
        { cx - halfLen + ht, cy - ht },
        { cx + halfLen - ht, cy - ht },
        { cx + halfLen, cy },
        { cx + halfLen - ht, cy + ht },
        { cx - halfLen + ht, cy + ht },
    };
    Polygon(hdc, pts, 6);
}

// 수직 육각형 세그먼트 (중심 cx, cy)
static void DrawVSegment(HDC hdc, int cx, int cy, int halfLen, int ht) {
    POINT pts[6] = {
        { cx, cy - halfLen },
        { cx + ht, cy - halfLen + ht },
        { cx + ht, cy + halfLen - ht },
        { cx, cy + halfLen },
        { cx - ht, cy + halfLen - ht },
        { cx - ht, cy - halfLen + ht },
    };
    Polygon(hdc, pts, 6);
}

// 단일 숫자 그리기
static void DrawDigit(HDC hdc, int x, int y, int w, int h,
                      int thick, int gap, int digit) {
    BYTE seg = kSegDigits[digit % 10];
    int ht = thick / 2;

    int hCx = x + w / 2;
    int hHL = w / 2 - ht - ht / 3 - gap;  // 수평 세그먼트 약간 짧게

    int lCx = x + ht;
    int rCx = x + w - ht;

    int tTop = y + thick - ht / 3;
    int tBot = y + h / 2 - ht / 3;
    int tCy  = (tTop + tBot) / 2;
    int tHL  = (tBot - tTop) / 2;

    int bTop = y + h / 2 + ht / 3;
    int bBot = y + h - thick + ht / 3;
    int bCy  = (bTop + bBot) / 2;
    int bHL  = (bBot - bTop) / 2;

    if (seg & 0x01) DrawHSegment(hdc, hCx, y + ht + ht / 3, hHL, ht);
    if (seg & 0x02) DrawVSegment(hdc, rCx, tCy, tHL, ht);
    if (seg & 0x04) DrawVSegment(hdc, rCx, bCy, bHL, ht);
    if (seg & 0x08) DrawHSegment(hdc, hCx, y + h - ht - ht / 3, hHL, ht);
    if (seg & 0x10) DrawVSegment(hdc, lCx, bCy, bHL, ht);
    if (seg & 0x20) DrawVSegment(hdc, lCx, tCy, tHL, ht);
    if (seg & 0x40) DrawHSegment(hdc, hCx, y + h / 2, hHL, ht);
}

// 콜론 그리기 (상하 두 점)
static void DrawColon(HDC hdc, int cx, int y, int h, int dotR) {
    int y1 = y + h / 3;
    int y2 = y + 2 * h / 3;
    Ellipse(hdc, cx - dotR, y1 - dotR, cx + dotR, y1 + dotR);
    Ellipse(hdc, cx - dotR, y2 - dotR, cx + dotR, y2 + dotR);
}

void Draw7SegClock(HDC hdc, int bmpW, int bmpH,
                   const SegmentDisplayConfig& cfg,
                   int hour, int minute) {
    int dw = cfg.digitW;
    int dh = cfg.digitH;
    int thick = cfg.segThick;
    int gap = cfg.segGap;
    int spacing = cfg.digitSpacing;
    int colW = cfg.colonW;

    int totalW = 4 * dw + colW + 4 * spacing;
    int sx = (bmpW - totalW) / 2;
    int sy = (bmpH - dh) / 2;

    COLORREF dimC = RGB(cfg.dimBrightness, cfg.dimBrightness, cfg.dimBrightness);
    HBRUSH dimBr = CreateSolidBrush(dimC);
    HPEN dimPn = CreatePen(PS_SOLID, 1, dimC);

    // 패스 1: 모든 세그먼트를 어둡게 그리기 (숫자 "8" = 0x7F = 전체 점등)
    HGDIOBJ oldBr = SelectObject(hdc, dimBr);
    HGDIOBJ oldPn = SelectObject(hdc, dimPn);

    int x = sx;
    DrawDigit(hdc, x, sy, dw, dh, thick, gap, 8);
    x += dw + spacing;
    DrawDigit(hdc, x, sy, dw, dh, thick, gap, 8);
    x += dw + spacing;
    DrawColon(hdc, x + colW / 2, sy, dh, thick / 2);
    x += colW + spacing;
    DrawDigit(hdc, x, sy, dw, dh, thick, gap, 8);
    x += dw + spacing;
    DrawDigit(hdc, x, sy, dw, dh, thick, gap, 8);

    // 패스 2: 활성 세그먼트를 밝게 덮어 그리기
    SelectObject(hdc, GetStockObject(WHITE_BRUSH));
    SelectObject(hdc, GetStockObject(WHITE_PEN));

    x = sx;
    DrawDigit(hdc, x, sy, dw, dh, thick, gap, hour / 10);
    x += dw + spacing;
    DrawDigit(hdc, x, sy, dw, dh, thick, gap, hour % 10);
    x += dw + spacing;
    DrawColon(hdc, x + colW / 2, sy, dh, thick / 2);
    x += colW + spacing;
    DrawDigit(hdc, x, sy, dw, dh, thick, gap, minute / 10);
    x += dw + spacing;
    DrawDigit(hdc, x, sy, dw, dh, thick, gap, minute % 10);

    SelectObject(hdc, oldBr);
    SelectObject(hdc, oldPn);
    DeleteObject(dimBr);
    DeleteObject(dimPn);
}

} // namespace wsse
