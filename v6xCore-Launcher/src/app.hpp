#pragma once
#include "graphics.hpp"
#include "overlay.hpp"
#include "config_name.hpp"
#include "bootstrap.hpp"
#include "updater.hpp"
#include "capture_privacy.hpp"
#include "frame_clock.hpp"
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <shellapi.h>
#include <sstream>
#include <vector>

namespace v6x {
struct LogLine {
    std::string time, message;
};
struct App {
    HWND window{}, overlay{}, game{};
    Graphics graphics;
    OverlayPainter painter;
    cs2::Config config;
    cs2::Offsets offsets;
    cs2::Process process;
    cs2::GameReader reader{process, offsets};
    cs2::AimController aim;
    cs2::Frame frame;
    std::filesystem::path executable = exeDirectory();
    std::filesystem::path directory;
    OffsetUpdater updater;
    bool restoredAtStartup = false;
    bool licensed = false;
    CapturePrivacy capturePrivacy;
    double lastStep{}, rateWindow{};
    unsigned sampleCount{};
    FrameClock overlayClock;
    std::vector<std::string> configs;
    std::vector<LogLine> logs;
    std::string selectedConfig = "Last settings", status = "Not connected", toast, lastLogState;
    bool connected = false, paused = false, active = false, dirty = false, testMode = false, quit = false;
    ULONGLONG toastUntil{}, modifiedAt{}, lastAttach{}, lastSample{}, lastFind{}, lastTick{}, lastRender{};
    RECT overlayRect{};
    float sampleMs{}, measuredFps{};
    std::array<bool, 256> keys{};
    explicit App(bool test, const std::filesystem::path &dataOverride = {})
        : directory(dataDirectory(executable, dataOverride)), testMode(test) {
        auto start = bootstrap(directory);
        restoredAtStartup = start.offsetsCreated;
        config.load((directory / "settings.ini").wstring());
        offsets.load((directory / "offsets.ini").wstring());
        refreshConfigs();
        ensureUtf8Bom(directory / "v6xcore.log");
        log("v6xCore 3.0 started. User mode / no driver.");
        log("Data folder: " + utf8(directory.wstring()));
        for (const auto &message : start.messages)
            log(message);
        if (!offsets.valid)
            log("Offset load error: " + utf8(offsets.error));
        else
            log("Offsets loaded: " + utf8(offsets.provider) + " / Build " + std::to_string(offsets.build));
        connected = false;
        status = testMode         ? "UI preview: process access disabled"
                 : !offsets.valid ? utf8(offsets.error)
                 : connected      ? "Looking for cs2.exe ..."
                                  : "Disconnected. Select Connect above.";
        log(status);
    }
    ~App() {
        if (dirty)
            config.save();
    }
    void log(const std::string &message) {
        SYSTEMTIME now{};
        GetLocalTime(&now);
        char t[16]{};
        std::snprintf(t, sizeof(t), "%02u:%02u:%02u", now.wHour, now.wMinute, now.wSecond);
        logs.push_back({t, message});
        if (logs.size() > 200)
            logs.erase(logs.begin());
        try {
            auto file = directory / "v6xcore.log";
            if (std::filesystem::exists(file) && std::filesystem::file_size(file) > 1024 * 1024)
                atomicWrite(file, "\xef\xbb\xbf");
            std::ofstream stream(file, std::ios::app | std::ios::binary);
            stream << t << "  " << message << '\n';
        } catch (...) {
        }
    }
    void notify(const std::string &text) {
        toast = text;
        toastUntil = GetTickCount64() + 3200;
        log(text);
    }
    void change() {
        dirty = true;
        modifiedAt = GetTickCount64();
    }
    void save() {
        if (config.save()) {
            dirty = false;
        } else
            notify("Settings could not be saved.");
    }
    void reload() {
        aim.reset();
        frame = {};
        reader.reset();
        config.load((directory / "settings.ini").wstring());
        offsets.load((directory / "offsets.ini").wstring());
        dirty = false;
        notify(offsets.valid ? "Files reloaded. Aim control is off." : utf8(offsets.error));
    }
    void connect() {
        if (!testMode && !licensed) {
            notify("A verified license is required to open this module.");
            return;
        }
        if (testMode) {
            notify("Connection is disabled in UI preview mode.");
            return;
        }
        if (!offsets.valid) {
            notify("Load valid offsets first: " + utf8(offsets.error));
            return;
        }
        connected = !connected;
        aim.reset();
        frame = {};
        reader.reset();
        config.aim = false;
        if (!connected) {
            process.close();
            game = nullptr;
            active = false;
            status = "Not connected";
            notify("Disconnected.");
        } else {
            lastAttach = 0;
            status = "Waiting for cs2.exe";
            notify("Requested normal user-mode read access.");
        }
    }
    void pause() {
        paused = !paused;
        aim.reset();
        frame = {};
        notify(paused ? "All output paused." : "Output resumed.");
    }
    void refreshConfigs() {
        configs.clear();
        std::error_code e;
        std::filesystem::create_directories(directory / "configs", e);
        for (std::filesystem::directory_iterator it(directory / "configs", e), end; !e && it != end;
             it.increment(e)) {
            if (it->is_regular_file(e) && it->path().extension() == ".ini") {
                auto name = it->path().stem().string();
                if (cs2::safeConfigName(name))
                    configs.push_back(name);
            }
        }
        std::sort(configs.begin(), configs.end());
    }
    bool loadConfig(const std::string &name) {
        auto safe = cs2::safeConfigName(name);
        if (!safe)
            return false;
        auto file = directory / "configs" / (*safe + ".ini");
        if (!std::filesystem::is_regular_file(file))
            return false;
        config.load(file.wstring());
        config.path = (directory / "settings.ini").wstring();
        selectedConfig = *safe;
        aim.reset();
        save();
        notify("Config loaded: " + *safe + ". Aim control stays off.");
        return true;
    }
    bool saveConfig(const std::string &name) {
        auto safe = cs2::safeConfigName(name);
        if (!safe) {
            notify("Config name: 1-40 characters, A-Z, 0-9, spaces, - or _.");
            return false;
        }
        if (!config.saveAs((directory / "configs" / (*safe + ".ini")).wstring())) {
            notify("Config could not be saved.");
            return false;
        }
        selectedConfig = *safe;
        refreshConfigs();
        save();
        notify("Config saved: " + *safe);
        return true;
    }
    void removeConfig(const std::string &name) {
        auto safe = cs2::safeConfigName(name);
        if (!safe)
            return;
        std::error_code e;
        bool removed = std::filesystem::remove(directory / "configs" / (*safe + ".ini"), e);
        refreshConfigs();
        notify(removed && !e ? "Config deleted: " + *safe : "Config could not be deleted.");
    }
    void resetSettings() {
        auto path = config.path;
        config = cs2::Config{};
        config.path = path;
        aim.reset();
        change();
        notify("Settings reset. Aim control is off.");
    }
    void open(const std::filesystem::path &path) {
        std::error_code error;
        if (!std::filesystem::exists(path, error)) {
            notify("File/folder is missing: " + utf8(path.wstring()));
            return;
        }
        const auto result = reinterpret_cast<INT_PTR>(
            ShellExecuteW(window, L"open", path.c_str(), nullptr, directory.c_str(), SW_SHOWNORMAL));
        if (result <= 32)
            notify("Could not open (" + std::to_string(result) + "): " + utf8(path.wstring()));
    }
    void startUpdate() {
        if (updater.busy())
            return;
        config.aim = false;
        aim.reset();
        updater.start(directory);
        notify("HTTPS offset update started. No Python required.");
    }
    void restoreOffsets() {
        if (updater.busy())
            return;
        try {
            restoreBundledOffsets(directory);
            offsets.load((directory / "offsets.ini").wstring());
            frame = {};
            reader.reset();
            aim.reset();
            config.aim = false;
            if (!testMode && config.autoConnect && offsets.valid) {
                connected = true;
                lastAttach = 0;
            }
            notify("Bundled offsets restored; previous file backed up.");
        } catch (const std::exception &error) {
            notify(error.what());
        }
    }
    std::string overlayStatus() const {
        if (testMode)
            return "UI preview: no game overlay or input.";
        if (!offsets.valid)
            return "Hidden: offset file is missing or invalid.";
        if (!connected)
            return "Hidden: not connected. Select Connect.";
        if (paused)
            return "Hidden: output is paused.";
        if (!process.alive())
            return "Hidden: no readable CS2 process.";
        if (!IsWindow(game))
            return "Hidden: CS2 window not found.";
        if (IsIconic(game))
            return "Hidden: CS2 is minimized.";
        if (!frame.valid)
            return "Hidden: " + utf8(reader.status);
        if (!active)
            return "Hidden: CS2 is not in the foreground.";
        if (!config.esp && !config.fovRing && !config.crosshair && !config.watermark)
            return "Transparent: all overlay elements are off.";
        return IsWindowVisible(overlay) ? "Overlay window visible; live functionality unverified."
                                        : "Overlay is waiting for the next frame.";
    }
    void openUrl(const wchar_t *url) {
        auto result =
            reinterpret_cast<INT_PTR>(ShellExecuteW(window, L"open", url, nullptr, nullptr, SW_SHOWNORMAL));
        if (result <= 32)
            notify("Could not open the external link. Windows code " + std::to_string(result));
    }
    void diagnostics() {
        const auto file = directory / "diagnostics.txt";
        std::ofstream out(file, std::ios::binary);
        if (!out) {
            notify("Could not save the diagnostic report: " + utf8(file.wstring()));
            return;
        }
        out << "\xef\xbb\xbf" << "v6xCore Cs2 External 3.0\nUser Mode only; no driver\n\nStatus: " << status
            << "\nConnection requested: " << connected << "\nAuto-connect setting: " << config.autoConnect
            << "\nUI/test mode: " << testMode << "\nExpected process: cs2.exe"
            << "\nPID detected: " << process.foundPid() << "\nPID attached: " << process.pid()
            << "\nLast Windows error: " << process.lastError()
            << "\nExecutable directory: " << utf8(executable.wstring())
            << "\nData directory: " << utf8(directory.wstring())
            << "\nCurrent working directory: " << utf8(std::filesystem::current_path().wstring())
            << "\nOffset path: " << utf8((directory / "offsets.ini").wstring())
            << "\nOffsets valid: " << offsets.valid << "\nOffset load error: " << utf8(offsets.error)
            << "\nBundled offsets recovered at start: " << restoredAtStartup
            << "\nRuntime layout validated: " << (reader.stride() != 0) << "\nEntity stride: 0x" << std::hex
            << reader.stride() << std::dec << "\nSource: " << utf8(offsets.sourceUrl)
            << "\nCommit: " << utf8(offsets.sourceCommit) << "\nSource date: " << utf8(offsets.sourceDate)
            << "\nSnapshot build: " << offsets.build << "\nRetrieved UTC: " << utf8(offsets.retrievedDate)
            << "\nSample time: " << sampleMs << " ms\nSampling rate: " << measuredFps
            << " Hz\nRequested sampling: " << config.sampleHz
            << " Hz\nRequested overlay rate: " << config.overlayFps << " Hz\nFFA mode: " << config.freeForAll
            << "\nTarget teammates: " << config.targetTeammates
            << "\nCapture privacy: " << capturePrivacy.status()
            << "\nLicense gate: " << (licensed ? "Authorized" : "Not authorized")
            << "\nOverlay status: " << overlayStatus()
            << "\n\nCompatibility with live CS2 is not verified.\n\nLog:\n";
        for (const auto &entry : logs)
            out << entry.time << " " << entry.message << '\n';
        out.close();
        notify("diagnostics.txt saved: " + utf8(file.wstring()));
    }
    void hotkeys() {
        const bool editing =
            graphics.initialized && (ImGui::GetIO().WantTextInput || ImGui::IsAnyItemActive() ||
                                     ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup));
        bool accept = active || (cs2::foregroundIs(GetCurrentProcessId()) && !editing);
        auto pressed = [&](int key) {
            bool down = (GetAsyncKeyState(key) & 0x8000) != 0;
            bool p = accept && down && !keys[key];
            keys[key] = down;
            return p;
        };
        if (pressed(VK_F1)) {
            config.esp = !config.esp;
            change();
        }
        if (pressed(VK_F2)) {
            config.aim = !config.aim;
            aim.reset();
            change();
        }
        if (pressed(VK_F3)) {
            config.boxes = !config.boxes;
            change();
        }
        if (pressed(VK_F4)) {
            config.names = !config.names;
            change();
        }
        if (pressed(VK_F5)) {
            config.health = !config.health;
            change();
        }
        if (pressed(VK_F6)) {
            config.teammates = !config.teammates;
            change();
        }
        if (pressed(VK_F7)) {
            config.fovRing = !config.fovRing;
            change();
        }
        if (pressed(VK_F8)) {
            config.spottedOnly = !config.spottedOnly;
            change();
        }
        if (pressed(VK_F9))
            reload();
        if (pressed(VK_PAUSE))
            pause();
        if (pressed(VK_INSERT)) {
            if (IsWindowVisible(window) && !IsIconic(window))
                ShowWindow(window, SW_MINIMIZE);
            else {
                ShowWindow(window, SW_RESTORE);
                SetForegroundWindow(window);
            }
        }
        if (pressed(VK_END))
            PostMessageW(window, WM_CLOSE, 0, 0);
    }
    void maintenance() {
        UpdateResult updateResult;
        if (updater.take(updateResult)) {
            if (updateResult.success) {
                offsets.load((directory / "offsets.ini").wstring());
                frame = {};
                reader.reset();
                aim.reset();
                config.aim = false;
            }
            notify(updateResult.message);
        }
        ULONGLONG now = GetTickCount64();
        if (dirty && now - modifiedAt >= 750)
            save();
    }
    void stopOutput() {
        connected = false;
        active = false;
        config.aim = false;
        frame = {};
        aim.reset();
        reader.reset();
        process.close();
        game = nullptr;
        if (IsWindow(overlay))
            ShowWindow(overlay, SW_HIDE);
    }
    void tick() {
        if (!testMode && !licensed) {
            stopOutput();
            status = "License required. Return to the launcher.";
            return;
        }
        ULONGLONG now = GetTickCount64();
        double precise = secondsNow();
        float dt = lastStep ? std::clamp(static_cast<float>(precise - lastStep), .0005f, .1f) : .016f;
        lastStep = precise;
        if (connected && !testMode && !process.alive() && (!lastAttach || now - lastAttach >= 1000)) {
            lastAttach = now;
            reader.reset();
            frame = {};
            game = nullptr;
            process.attach();
        }
        if (connected && process.alive() && (!IsWindow(game) || now - lastFind >= 2000)) {
            game = cs2::gameWindow(process.pid());
            lastFind = now;
        }
        active = connected && process.alive() && IsWindow(game) && !IsIconic(game) &&
                 cs2::foregroundIs(process.pid());
        hotkeys();
        if (!connected) {
            frame = {};
            status = testMode         ? "UI preview: process access disabled"
                     : !offsets.valid ? utf8(offsets.error)
                                      : "Disconnected. Select Connect above.";
        } else if (paused) {
            frame = {};
            status = "Paused";
        } else if (!offsets.valid) {
            frame = {};
            status = utf8(offsets.error);
        } else if (!process.alive()) {
            frame = {};
            status = utf8(process.status);
        } else if (active || !lastSample || now - lastSample >= 250) {
            const auto start = std::chrono::steady_clock::now();
            lastSample = now;
            ++sampleCount;
            if (!rateWindow)
                rateWindow = precise;
            if (precise - rateWindow >= .5) {
                measuredFps = static_cast<float>(sampleCount / (precise - rateWindow));
                sampleCount = 0;
                rateWindow = precise;
            }
            reader.sample(frame);
            sampleMs =
                std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - start).count();
            status = utf8(reader.status);
        }
        aim.update(frame, config, active && !paused && !testMode, dt);
        if (aim.inputRejected) {
            config.aim = false;
            aim.reset();
            notify("SendInput was rejected. Aim control disabled.");
        }
        if (active && !paused && frame.valid) {
            RECT bounds{};
            POINT pos{};
            if (GetClientRect(game, &bounds) && ClientToScreen(game, &pos) && bounds.right > 0 &&
                bounds.bottom > 0) {
                RECT next{pos.x, pos.y, pos.x + bounds.right, pos.y + bounds.bottom};
                if (!EqualRect(&next, &overlayRect) || !IsWindowVisible(overlay)) {
                    overlayRect = next;
                    SetWindowPos(overlay, HWND_TOPMOST, pos.x, pos.y, bounds.right, bounds.bottom,
                                 SWP_NOACTIVATE | SWP_SHOWWINDOW);
                }
                SetLayeredWindowAttributes(overlay, RGB(0, 0, 0), static_cast<BYTE>(config.opacity * 255),
                                           LWA_COLORKEY | LWA_ALPHA);
                if (overlayClock.due(precise, config.overlayFps)) {
                    lastRender = now;
                    InvalidateRect(overlay, nullptr, FALSE);
                }
            } else
                ShowWindow(overlay, SW_HIDE);
        } else
            ShowWindow(overlay, SW_HIDE);
        const std::string key = frame.valid ? "Live data received with validated entity layout." : status;
        if (key != lastLogState) {
            lastLogState = key;
            log(key);
        }
    }
};
} // namespace v6x
