#pragma once
#include "game.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace v6x {
inline COLORREF color(const std::array<float, 3> &v) {
    return RGB(static_cast<int>(v[0] * 255), static_cast<int>(v[1] * 255), static_cast<int>(v[2] * 255));
}
inline void fill(HDC dc, int x, int y, int w, int h, COLORREF c) {
    RECT r{x, y, x + w, y + h};
    HBRUSH b = CreateSolidBrush(c);
    FillRect(dc, &r, b);
    DeleteObject(b);
}
inline void line(HDC dc, int x, int y, int x2, int y2, COLORREF c, int width = 1) {
    HPEN pen = CreatePen(PS_SOLID, width, c);
    auto old = SelectObject(dc, pen);
    MoveToEx(dc, x, y, nullptr);
    LineTo(dc, x2, y2);
    SelectObject(dc, old);
    DeleteObject(pen);
}
inline void box(HDC dc, int x, int y, int w, int h, COLORREF c, int width, int style) {
    if (style == 1) {
        const int a = std::max(3, w / 4), b = std::max(3, h / 5);
        for (int sx : {0, 1})
            for (int sy : {0, 1}) {
                int px = x + sx * w, py = y + sy * h;
                line(dc, px, py, px + (sx ? -a : a), py, c, width);
                line(dc, px, py, px, py + (sy ? -b : b), c, width);
            }
        return;
    }
    HPEN pen = CreatePen(PS_SOLID, width, c);
    auto op = SelectObject(dc, pen), ob = SelectObject(dc, GetStockObject(NULL_BRUSH));
    if (style == 2)
        RoundRect(dc, x, y, x + w, y + h, 8, 8);
    else
        Rectangle(dc, x, y, x + w, y + h);
    SelectObject(dc, op);
    SelectObject(dc, ob);
    DeleteObject(pen);
}
inline void circle(HDC dc, int x, int y, int r, COLORREF c) {
    HPEN pen = CreatePen(PS_SOLID, 1, c);
    auto op = SelectObject(dc, pen), ob = SelectObject(dc, GetStockObject(NULL_BRUSH));
    Ellipse(dc, x - r, y - r, x + r, y + r);
    SelectObject(dc, op);
    SelectObject(dc, ob);
    DeleteObject(pen);
}
inline std::wstring fixed(float v, int precision = 0) {
    std::wostringstream s;
    s << std::fixed << std::setprecision(precision) << v;
    return s.str();
}
struct OverlayPainter {
    HFONT font{};
    int size{};
    HDC backDC{};
    HBITMAP backBitmap{};
    HGDIOBJ previousBitmap{};
    int backWidth{}, backHeight{};
    void releaseBuffer() {
        if (backDC && previousBitmap)
            SelectObject(backDC, previousBitmap);
        if (backBitmap)
            DeleteObject(backBitmap);
        if (backDC)
            DeleteDC(backDC);
        backDC = nullptr;
        backBitmap = nullptr;
        previousBitmap = nullptr;
    }
    ~OverlayPainter() {
        releaseBuffer();
        if (font)
            DeleteObject(font);
    }
    void label(HDC dc, const std::wstring &str, int x, int y, int w, COLORREF c, bool centered = true) {
        SetBkMode(dc, TRANSPARENT);
        SelectObject(dc, font);
        const UINT flags =
            (centered ? DT_CENTER : DT_LEFT) | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX;
        RECT r{x + 1, y + 1, x + w + 1, y + size + 5};
        SetTextColor(dc, RGB(1, 1, 1));
        DrawTextW(dc, str.c_str(), static_cast<int>(str.size()), &r, flags);
        r = {x, y, x + w, y + size + 4};
        SetTextColor(dc, c);
        DrawTextW(dc, str.c_str(), static_cast<int>(str.size()), &r, flags);
    }
    void paint(HWND window, HDC destination, const cs2::Frame &frame, const cs2::Config &cfg,
               cs2::Address target, float fps) {
        using namespace cs2;
        RECT r{};
        GetClientRect(window, &r);
        const int width = r.right, height = r.bottom;
        if (width <= 0 || height <= 0)
            return;
        if (!backDC || width != backWidth || height != backHeight) {
            releaseBuffer();
            backDC = CreateCompatibleDC(destination);
            backBitmap = CreateCompatibleBitmap(destination, width, height);
            if (!backDC || !backBitmap) {
                releaseBuffer();
                return;
            }
            previousBitmap = SelectObject(backDC, backBitmap);
            backWidth = width;
            backHeight = height;
        }
        HDC dc = backDC;
        RECT clear{0, 0, width, height};
        FillRect(dc, &clear, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
        const int nextSize = static_cast<int>(cfg.textSize);
        if (!font || nextSize != size) {
            if (font)
                DeleteObject(font);
            size = nextSize;
            font = CreateFontW(-size, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                               OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH,
                               L"Segoe UI");
        }
        if (frame.valid) {
            if (cfg.fovRing) {
                const auto &m = frame.matrix.m;
                float radius = width * 0.5f *
                               std::sqrt(m[0][0] * m[0][0] + m[0][1] * m[0][1] + m[0][2] * m[0][2]) *
                               std::tan(rad(cfg.fov));
                if (std::isfinite(radius) && radius > 1 && radius < std::max(width, height) * 2)
                    circle(dc, width / 2, height / 2, static_cast<int>(radius), color(cfg.accent));
            }
            if (cfg.crosshair) {
                const int s = static_cast<int>(cfg.crosshairSize), x = width / 2, y = height / 2;
                const auto c = color(cfg.accent);
                line(dc, x - s - 2, y, x - 2, y, c);
                line(dc, x + 2, y, x + s + 2, y, c);
                line(dc, x, y - s - 2, x, y - 2, c);
                line(dc, x, y + 2, x, y + s + 2, c);
            }
            if (cfg.esp)
                for (const auto &p : frame.players) {
                    const bool team = cs2::sameSide(frame.team, p.team, cfg.freeForAll);
                    const float meters = (p.feet - frame.eye).length() * 0.0254f;
                    if ((team && !cfg.teammates) || meters > cfg.maxDistance)
                        continue;
                    auto feet = project(p.feet - Vec3{0, 0, 3}, frame.matrix, width, height),
                         top = project(p.top, frame.matrix, width, height);
                    if (!feet || !top)
                        continue;
                    const float raw = feet->y - top->y;
                    if (raw < 4 || raw > height * 3)
                        continue;
                    const int h = static_cast<int>(raw), w = static_cast<int>(raw * 0.43f),
                              x = static_cast<int>((top->x + feet->x) / 2) - w / 2,
                              y = static_cast<int>(top->y);
                    const auto c = color(target == p.pawn ? cfg.targetColor
                                         : team           ? cfg.allyColor
                                                          : cfg.enemyColor);
                    const int stroke = std::max(1, static_cast<int>(std::lround(cfg.thickness)));
                    if (cfg.snaplines) {
                        int sy = cfg.lineOrigin == 0 ? 0 : cfg.lineOrigin == 1 ? height / 2 : height;
                        line(dc, width / 2, sy, x + w / 2, y + h, c);
                    }
                    if (cfg.boxes) {
                        if (cfg.outlines)
                            box(dc, x, y, w, h, RGB(1, 1, 1), stroke + 2, cfg.boxStyle);
                        box(dc, x, y, w, h, c, stroke, cfg.boxStyle);
                    }
                    const int lw = std::max(160, w), lx = x + w / 2 - lw / 2;
                    if (cfg.names)
                        label(dc, p.name, lx, y - size - 8, lw, c);
                    if (cfg.health) {
                        const int hh = static_cast<int>(h * std::clamp(p.health / 100.f, 0.f, 1.f));
                        fill(dc, x - 8, y - 1, 5, h + 2, RGB(1, 1, 1));
                        fill(dc, x - 7, y + h - hh, 3, hh,
                             p.health > 35 ? RGB(116, 235, 184) : RGB(255, 114, 120));
                    }
                    std::wstring details;
                    if (cfg.healthText)
                        details = std::to_wstring(p.health) + L" HP";
                    if (cfg.distance) {
                        if (!details.empty())
                            details += L"  |  ";
                        details += fixed(meters) + L" m";
                    }
                    if (!details.empty())
                        label(dc, details, lx, y + h + 4, lw, RGB(222, 225, 231));
                    if (cfg.headMarker || target == p.pawn) {
                        auto head = project(p.aimPoint, frame.matrix, width, height);
                        if (head)
                            circle(dc, static_cast<int>(head->x), static_cast<int>(head->y),
                                   target == p.pawn ? 5 : 3, c);
                    }
                }
            if (cfg.watermark) {
                fill(dc, 15, height - 58, 370, 42, RGB(17, 17, 22));
                label(dc, L"v6xCore  |  Cs2 External", 26, height - 55, 348, color(cfg.accent), false);
                std::wstring state = L"ESP " + std::wstring(cfg.esp ? L"ON" : L"OFF") + L"  |  AIM " +
                                     (cfg.aim ? L"ARMED" : L"OFF");
                if (cfg.showFps)
                    state += L"  |  " + fixed(fps) + L" Hz";
                label(dc, state, 26, height - 35, 348, RGB(146, 149, 160), false);
            }
        }
        BitBlt(destination, 0, 0, width, height, dc, 0, 0, SRCCOPY);
        SelectObject(dc, GetStockObject(SYSTEM_FONT));
    }
};
} // namespace v6x
