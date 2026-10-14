#include "launcher.hpp"
#include <cstdio>
#include <dwmapi.h>
#include <windowsx.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);
using v6x::App;
namespace {
App *application(HWND w) {
    return reinterpret_cast<App *>(GetWindowLongPtrW(w, GWLP_USERDATA));
}
LRESULT CALLBACK panelProc(HWND w, UINT msg, WPARAM wp, LPARAM lp) {
    App *app = application(w);
    if (msg == WM_NCCREATE) {
        app = static_cast<App *>(reinterpret_cast<CREATESTRUCTW *>(lp)->lpCreateParams);
        SetWindowLongPtrW(w, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        app->window = w;
    }
    if (app && app->graphics.initialized && ImGui_ImplWin32_WndProcHandler(w, msg, wp, lp))
        return TRUE;
    switch (msg) {
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps{};
        BeginPaint(w, &ps);
        EndPaint(w, &ps);
        return 0;
    }
    case WM_SIZE:
        if (app && app->graphics.initialized && wp != SIZE_MINIMIZED)
            app->graphics.resize(LOWORD(lp), HIWORD(lp));
        return 0;
    case WM_NCHITTEST: {
        POINT p{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        ScreenToClient(w, &p);
        RECT r{};
        GetClientRect(w, &r);
        float s = r.right / 980.f;
        if (p.y >= 0 && p.y < 70 * s && p.x >= 0 && p.x < 490 * s)
            return HTCAPTION;
        return HTCLIENT;
    }
    case WM_DPICHANGED: {
        auto *r = reinterpret_cast<RECT *>(lp);
        SetWindowPos(w, nullptr, r->left, r->top, r->right - r->left, r->bottom - r->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        return 0;
    }
    case WM_SYSCOMMAND:
        if ((wp & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_CLOSE:
        DestroyWindow(w);
        return 0;
    case WM_DESTROY:
        if (app) {
            app->quit = true;
            app->aim.reset();
            if (app->dirty)
                app->save();
            if (IsWindow(app->overlay))
                DestroyWindow(app->overlay);
        }
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }
    return DefWindowProcW(w, msg, wp, lp);
}
LRESULT CALLBACK overlayProc(HWND w, UINT msg, WPARAM wp, LPARAM lp) {
    App *app = application(w);
    if (msg == WM_NCCREATE) {
        app = static_cast<App *>(reinterpret_cast<CREATESTRUCTW *>(lp)->lpCreateParams);
        SetWindowLongPtrW(w, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        app->overlay = w;
    }
    switch (msg) {
    case WM_NCHITTEST:
        return HTTRANSPARENT;
    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC dc = BeginPaint(w, &ps);
        if (app)
            app->painter.paint(w, dc, app->frame, app->config, app->aim.target, app->measuredFps);
        EndPaint(w, &ps);
        return 0;
    }
    default:
        return DefWindowProcW(w, msg, wp, lp);
    }
}
} // namespace
int runApplication(HINSTANCE instance, int show) {
    bool uiTest = false, selfCheck = false, updateOnly = false, repairOnly = false, authCheck = false;
    std::filesystem::path overrideDirectory;
    int argc = 0;
    LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv)
        throw std::runtime_error("Command line could not be parsed");
    for (int i = 1; i < argc; ++i) {
        std::wstring arg = argv[i];
        if (arg == L"--ui-test")
            uiTest = true;
        else if (arg == L"--auth-check")
            authCheck = true;
        else if (arg == L"--self-check")
            selfCheck = true;
        else if (arg == L"--update-offsets")
            updateOnly = true;
        else if (arg == L"--repair-offsets")
            repairOnly = true;
        else if (arg == L"--data-dir" && i + 1 < argc)
            overrideDirectory = argv[++i];
        else {
            LocalFree(argv);
            throw std::runtime_error("Unknown or incomplete command-line option");
        }
    }
    LocalFree(argv);
    bool headless = selfCheck || updateOnly || repairOnly || authCheck;
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    cs2::Handle instanceLock(CreateMutexW(nullptr, FALSE, L"Local\\v6xCoreCs2External02"));
    if (!instanceLock || GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBoxW(nullptr, L"v6xCore is already running.", L"v6xCore Launcher", MB_OK);
        if (SUCCEEDED(com))
            CoUninitialize();
        return 1;
    }
    int result = 0;
    {
        App app(uiTest || headless, overrideDirectory);
        if (headless) {
            if (authCheck) {
                v6x::licensing::Client auth(v6x::catalog[0]);
                auth.initialize(false);
                auth.wait();
                const auto state = auth.snapshot();
                std::printf("KEYAUTH_PUBLIC_INIT_%s\nNo license or application secret sent.\n",
                            state.initialized ? "OK" : "FAILED");
                if (!state.initialized)
                    std::printf("Error: %s / %s\n", state.errorCode.c_str(), state.message.c_str());
                std::fflush(stdout);
                if (SUCCEEDED(com))
                    CoUninitialize();
                return state.initialized ? 0 : 4;
            }
            if (repairOnly) {
                v6x::restoreBundledOffsets(app.directory);
                app.offsets.load((app.directory / "offsets.ini").wstring());
            }
            if (updateOnly) {
                auto update = v6x::updateOnline(app.directory, [](const std::string &status) {
                    std::puts(status.c_str());
                    std::fflush(stdout);
                });
                app.log(update.message);
                std::puts(update.message.c_str());
                if (!update.success)
                    result = 3;
                app.offsets.load((app.directory / "offsets.ini").wstring());
            }
            app.diagnostics();
            std::printf("V6X_SELF_CHECK_%s\nData directory: %s\nOffsets valid: %d\nSource: %s\nCommit: "
                        "%s\nBuild: %u\n",
                        app.offsets.valid ? "OK" : "FAILED", v6x::utf8(app.directory.wstring()).c_str(),
                        app.offsets.valid, v6x::utf8(app.offsets.sourceUrl).c_str(),
                        v6x::utf8(app.offsets.sourceCommit).c_str(), app.offsets.build);
            std::fflush(stdout);
            if (!app.offsets.valid)
                result = 2;
            if (SUCCEEDED(com))
                CoUninitialize();
            return result;
        }
        WNDCLASSEXW pc{};
        pc.cbSize = sizeof(pc);
        pc.hInstance = instance;
        pc.lpfnWndProc = panelProc;
        pc.lpszClassName = L"v6xCorePanel02";
        pc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        pc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(101));
        pc.hIconSm = pc.hIcon;
        WNDCLASSEXW oc = pc;
        oc.lpszClassName = L"v6xCoreOverlay02";
        oc.lpfnWndProc = overlayProc;
        if (!RegisterClassExW(&pc) || !RegisterClassExW(&oc)) {
            MessageBoxW(nullptr, L"Could not register window classes.", L"v6xCore", MB_OK | MB_ICONERROR);
            return 1;
        }
        RECT work{};
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
        float dpi = GetDpiForSystem() / 96.f;
        float fit =
            std::min({dpi, (work.right - work.left - 40) / 980.f, (work.bottom - work.top - 40) / 740.f});
        fit = std::max(.5f, fit);
        int width = static_cast<int>(980 * fit), height = static_cast<int>(740 * fit),
            x = work.left + (work.right - work.left - width) / 2,
            y = work.top + (work.bottom - work.top - height) / 2;
        app.window =
            CreateWindowExW(0, pc.lpszClassName, L"v6xCore Launcher", WS_POPUP | WS_SYSMENU | WS_MINIMIZEBOX,
                            x, y, width, height, nullptr, nullptr, instance, &app);
        app.overlay = CreateWindowExW(
            WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
            oc.lpszClassName, L"v6xCore Overlay", WS_POPUP, 0, 0, 1, 1, nullptr, nullptr, instance, &app);
        if (!app.window || !app.overlay ||
            !SetLayeredWindowAttributes(app.overlay, RGB(0, 0, 0), 255, LWA_COLORKEY)) {
            MessageBoxW(nullptr, L"Could not create the window or overlay.", L"v6xCore",
                        MB_OK | MB_ICONERROR);
            result = 1;
        } else if (!app.graphics.initialize(app.window)) {
            MessageBoxW(app.window, L"Could not initialize DirectX 11. Check your graphics drivers.",
                        L"v6xCore", MB_OK | MB_ICONERROR);
            result = 1;
        } else {
            const DWORD corner = 2;
            DwmSetWindowAttribute(app.window, 33, &corner, sizeof(corner));
            ShowWindow(app.window, show);
            UpdateWindow(app.window);
            v6x::Launcher launcher(app, uiTest);
            v6x::FrameClock sampleClock, uiClock;
            cs2::Handle timer(CreateWaitableTimerExW(nullptr, nullptr, 0x00000002, TIMER_ALL_ACCESS));
            if (!timer)
                timer.reset(CreateWaitableTimerW(nullptr, FALSE, nullptr));
            MSG message{};
            std::puts("V6X_UI_READY");
            std::fflush(stdout);
            while (!app.quit) {
                while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
                    if (message.message == WM_QUIT) {
                        app.quit = true;
                        break;
                    }
                    TranslateMessage(&message);
                    DispatchMessageW(&message);
                }
                if (app.quit)
                    break;
                const double now = v6x::secondsNow();
                launcher.update();
                app.maintenance();
                const bool module = launcher.page == v6x::Launcher::Page::Module;
                if (module && sampleClock.due(now, app.config.sampleHz))
                    app.tick();
                bool visible = IsWindowVisible(app.window) && !IsIconic(app.window);
                if (visible && uiClock.due(now, launcher.prefs.uiHz)) {
                    app.graphics.begin();
                    launcher.render();
                    app.graphics.present();
                }
                const double after = v6x::secondsNow();
                double wake = after + .03;
                if (visible)
                    wake = std::min(wake, uiClock.next);
                if (module)
                    wake = std::min(wake, sampleClock.next);
                const double remaining = std::max(.0001, wake - after);
                if (timer) {
                    LARGE_INTEGER due{};
                    due.QuadPart = -static_cast<LONGLONG>(remaining * 10000000.0);
                    SetWaitableTimer(timer.get(), &due, 0, nullptr, nullptr, FALSE);
                    HANDLE h = timer.get();
                    MsgWaitForMultipleObjectsEx(1, &h, 40, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
                } else
                    MsgWaitForMultipleObjectsEx(
                        0, nullptr, static_cast<DWORD>(std::max(1.0, std::ceil(remaining * 1000))),
                        QS_ALLINPUT, MWMO_INPUTAVAILABLE);
            }
            app.stopOutput();
            app.log("Application closed.");
        }
        if (IsWindow(app.window))
            DestroyWindow(app.window);
        if (IsWindow(app.overlay))
            DestroyWindow(app.overlay);
    }
    if (SUCCEEDED(com))
        CoUninitialize();
    return result;
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    try {
        return runApplication(instance, show);
    } catch (const std::exception &error) {
        std::fprintf(stderr, "v6xCore startup error: %s\n", error.what());
        MessageBoxW(nullptr, v6x::wide(error.what()).c_str(), L"v6xCore 3.0 - Startup error",
                    MB_OK | MB_ICONERROR);
        return 1;
    }
}
