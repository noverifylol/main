#pragma once
#include "memory.hpp"
#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_EXCLUDEFROMCAPTURE 0x00000011
#endif
namespace v6x {
struct CapturePrivacy {
    bool requested = false, applied = false, initialized = false;
    DWORD error{};
    void update(HWND panel, HWND overlay, bool enable) {
        if (initialized && requested == enable)
            return;
        requested = enable;
        initialized = true;
        applied = false;
        error = 0;
        DWORD wanted = enable ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE;
        BOOL a = SetWindowDisplayAffinity(panel, wanted);
        if (!a)
            error = GetLastError();
        BOOL b = SetWindowDisplayAffinity(overlay, wanted);
        if (!b && !error)
            error = GetLastError();
        DWORD panelValue{}, overlayValue{};
        bool read =
            GetWindowDisplayAffinity(panel, &panelValue) && GetWindowDisplayAffinity(overlay, &overlayValue);
        applied = a && b && read && panelValue == wanted && overlayValue == wanted;
        if (!applied && !error)
            error = GetLastError();
    }
    std::string status() const {
        if (!requested)
            return "Off";
        if (applied)
            return "Accepted by Windows; capture-method dependent";
        return "Not confirmed / Windows error " + std::to_string(error);
    }
};
} // namespace v6x
