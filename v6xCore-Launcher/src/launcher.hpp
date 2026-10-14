#pragma once
#include "ui.hpp"
#include "license_client.hpp"
#include "launcher_settings.hpp"
#include <cctype>

namespace v6x {
struct Launcher {
    enum class Page { Boot, License, Granted, Library, Module };
    App &app;
    UI core;
    LauncherSettings prefs;
    licensing::Client auth{catalog[0]};
    Page page = Page::Boot;
    double started = secondsNow(), grantedAt{};
    bool demo = false, settingsRequested = false, helpRequested = false, reveal = false, twoFactor = false;
    int filter = 0, detailGame = -1;
    float scale = 1;
    char key[256]{}, otp[16]{}, search[80]{};
    std::string localError;
    ImU32 white = IM_COL32(243, 243, 244, 255), muted = IM_COL32(139, 139, 145, 255),
          border = IM_COL32(49, 49, 52, 255);
    explicit Launcher(App &a, bool preview) : app(a), core(a), prefs(a.directory), demo(preview) {
        app.stopOutput();
        if (!demo)
            auth.initialize(prefs.remember);
    }
    ~Launcher() {
        SecureZeroMemory(key, sizeof(key));
        SecureZeroMemory(otp, sizeof(otp));
    }
    ImVec2 p(float x, float y) const {
        return {x * scale, y * scale};
    }
    void at(float x, float y) {
        ImGui::SetCursorPos(p(x, y));
    }
    void text(ImDrawList *d, float x, float y, const std::string &s, ImU32 color, float size = 14,
              ImFont *font = nullptr) {
        d->AddText(font ? font : app.graphics.body, size * scale, p(x, y), color, s.c_str());
    }
    void word(float x, float y, const std::string &s, ImU32 color, float size = 14, ImFont *font = nullptr) {
        text(ImGui::GetWindowDrawList(), x, y, s, color, size, font);
    }
    void wrapped(float x, float y, float width, const std::string &value, ImU32 color, float size = 13) {
        auto *d = ImGui::GetWindowDrawList();
        d->AddText(app.graphics.body, size * scale, p(x, y), color, value.c_str(), nullptr, width * scale);
    }
    void theme() {
        core.scale = scale;
        core.theme();
        auto &s = ImGui::GetStyle();
        auto gray = [](float v, float a = 1.f) { return ImVec4(v, v, v, a); };
        s.Colors[ImGuiCol_WindowBg] = gray(.035f);
        s.Colors[ImGuiCol_ChildBg] = gray(0, 0);
        s.Colors[ImGuiCol_Text] = gray(.94f);
        s.Colors[ImGuiCol_TextDisabled] = gray(.50f);
        s.Colors[ImGuiCol_Border] = gray(.18f);
        s.Colors[ImGuiCol_FrameBg] = gray(.085f);
        s.Colors[ImGuiCol_FrameBgHovered] = gray(.11f);
        s.Colors[ImGuiCol_FrameBgActive] = gray(.12f);
        s.Colors[ImGuiCol_Button] = gray(.12f);
        s.Colors[ImGuiCol_ButtonHovered] = gray(.19f);
        s.Colors[ImGuiCol_ButtonActive] = gray(.24f);
        s.Colors[ImGuiCol_CheckMark] = gray(.92f);
        s.Colors[ImGuiCol_Header] = gray(.3f, .35f);
        s.Colors[ImGuiCol_HeaderHovered] = gray(.3f, .5f);
        s.Colors[ImGuiCol_SliderGrab] = gray(.84f);
        s.Colors[ImGuiCol_SliderGrabActive] = gray(.97f);
        s.Colors[ImGuiCol_NavHighlight] = gray(.75f);
        s.Colors[ImGuiCol_TitleBg] = s.Colors[ImGuiCol_TitleBgActive] = gray(.085f);
        s.Colors[ImGuiCol_PopupBg] = gray(.06f);
        s.Colors[ImGuiCol_ModalWindowDimBg] = gray(0, .76f);
        s.FrameRounding = 7 * scale;
        s.PopupRounding = 12 * scale;
    }
    bool button(const char *label, float x, float y, float w, float h, bool primary = false) {
        at(x, y);
        if (primary) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(.93f, .93f, .94f, 1));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 1));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(.8f, .8f, .81f, 1));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(.045f, .045f, .05f, 1));
        }
        bool clicked = ImGui::Button(label, p(w, h));
        if (primary)
            ImGui::PopStyleColor(4);
        return clicked;
    }
    void pill(ImDrawList *d, float x, float y, float w, const char *label, bool strong = false) {
        d->AddRectFilled(p(x, y), p(x + w, y + 24),
                         strong ? IM_COL32(225, 225, 226, 255) : IM_COL32(35, 35, 38, 220), 12 * scale);
        text(d, x + 10, y + 5, label, strong ? IM_COL32(24, 24, 25, 255) : IM_COL32(193, 193, 198, 255), 10,
             app.graphics.medium);
    }
    void savePrefs() {
        if (!prefs.save())
            app.notify("Workspace preferences could not be saved.");
    }
    void header(bool library) {
        auto *d = ImGui::GetWindowDrawList();
        d->AddRectFilled(p(0, 0), p(980, 82), IM_COL32(11, 11, 12, 250));
        d->AddLine(p(24, 81), p(956, 81), IM_COL32(36, 36, 39, 255));
        app.graphics.drawLogo(d, p(20, 12), 56 * scale, prefs.animate);
        word(88, 20, "v6xCore", white, 23, app.graphics.medium);
        word(89, 50, library ? "YOUR WORKSPACE" : "ACCESS, SIMPLIFIED", muted, 9.5f, app.graphics.medium);
        if (library)
            pill(d, 530, 28, 128, demo ? "PREVIEW SESSION" : "LICENSE VERIFIED");
        if (button("Discord", 682, 23, 88, 34))
            app.openUrl(L"https://discord.gg/v6xcore");
        if (button("Settings", 780, 23, 91, 34))
            settingsRequested = true;
        if (button("-", 890, 24, 28, 30))
            ShowWindow(app.window, SW_MINIMIZE);
        if (button("x", 928, 24, 28, 30))
            PostMessageW(app.window, WM_CLOSE, 0, 0);
    }
    void footer() {
        auto *d = ImGui::GetWindowDrawList();
        d->AddRectFilled(p(0, 690), p(980, 740), IM_COL32(11, 11, 12, 255));
        d->AddLine(p(24, 690), p(956, 690), IM_COL32(35, 35, 38, 255));
        word(26, 708, demo ? "UI PREVIEW / NO PROCESS ACCESS" : "WINDOWS X64 / USER MODE", muted, 10,
             app.graphics.medium);
        word(798, 708, "v3.0 / MONO EDITION", muted, 10, app.graphics.medium);
    }
    void spinner(ImDrawList *d, ImVec2 center, float radius) {
        float phase = static_cast<float>(ImGui::GetTime() * 2.4);
        d->PathClear();
        d->PathArcTo(center, radius, phase, phase + 4.6f, 48);
        d->PathStroke(IM_COL32(221, 221, 225, 255), 0, 2 * scale);
    }
    static std::string expiryText(std::int64_t stamp) {
        if (stamp <= 0)
            return "Not available";
        std::time_t time = static_cast<std::time_t>(stamp);
        std::tm t{};
        gmtime_s(&t, &time);
        char text[40]{};
        std::strftime(text, sizeof(text), "%d %b %Y / UTC", &t);
        return text;
    }
    void update() {
        if (!demo)
            auth.tick();
        auto state = auth.snapshot();
        const double now = secondsNow();
        if (page == Page::Boot && now - started > 1.25 && (!state.busy || demo))
            page = state.authenticated ? Page::Granted : Page::License;
        if (page == Page::License && auth.authorized()) {
            page = Page::Granted;
            grantedAt = now;
            SecureZeroMemory(key, sizeof(key));
            SecureZeroMemory(otp, sizeof(otp));
            reveal = false;
        }
        if (page == Page::Granted) {
            if (!grantedAt)
                grantedAt = now;
            if (now - grantedAt > .85)
                page = Page::Library;
        }
        if (!demo && (page == Page::Module || page == Page::Library) && !auth.authorized()) {
            app.stopOutput();
            app.licensed = false;
            page = Page::License;
            localError = state.message.empty() ? "Your session needs verification. Reconnect to continue."
                                               : state.message;
        }
        app.licensed = !demo && auth.authorized() && page == Page::Module;
        app.capturePrivacy.update(app.window, app.overlay, prefs.capturePrivacy);
    }
    void openModule() {
        if (!demo && !auth.authorized()) {
            page = Page::License;
            return;
        }
        page = Page::Module;
        app.config.aim = false;
        app.aim.reset();
        app.paused = false;
        app.licensed = !demo && auth.authorized();
        app.connected = !demo && app.licensed && app.config.autoConnect && app.offsets.valid;
        app.lastAttach = 0;
        core.tab = 4;
    }
    void boot() {
        auto *d = ImGui::GetWindowDrawList();
        app.graphics.drawCover(d, 0, p(470, 0), p(980, 690), 95);
        d->AddRectFilledMultiColor(p(400, 0), p(980, 690), IM_COL32(9, 9, 10, 255), IM_COL32(9, 9, 10, 170),
                                   IM_COL32(9, 9, 10, 240), IM_COL32(9, 9, 10, 255));
        app.graphics.drawLogo(d, p(394, 173), 192 * scale, prefs.animate);
        word(369, 382, "v6xCore", white, 44, app.graphics.display);
        word(371, 443, "A cleaner way to get started.", muted, 14);
        auto state = auth.snapshot();
        spinner(d, p(384, 505), 8 * scale);
        word(405, 496, demo ? "Preparing interface preview" : state.stage, white, 12);
        footer();
    }
    void license() {
        auto *d = ImGui::GetWindowDrawList();
        app.graphics.drawCover(d, 0, p(380, 83), p(980, 690), 235);
        d->AddRectFilledMultiColor(p(375, 83), p(980, 690), IM_COL32(9, 9, 10, 255), IM_COL32(9, 9, 10, 20),
                                   IM_COL32(9, 9, 10, 120), IM_COL32(9, 9, 10, 255));
        d->AddRectFilledMultiColor(p(0, 420), p(980, 690), IM_COL32(9, 9, 10, 10), IM_COL32(9, 9, 10, 10),
                                   IM_COL32(9, 9, 10, 245), IM_COL32(9, 9, 10, 245));
        app.graphics.drawLogo(d, p(557, 377), 304 * scale, prefs.animate, 27);
        header(false);
        pill(d, 58, 119, 155, "COUNTER-STRIKE 2");
        word(56, 161, "Your access.", white, 44, app.graphics.display);
        word(56, 212, "Your workspace.", white, 44, app.graphics.display);
        word(59, 275, "Sign in with your game-specific license.", muted, 13);
        auto state = auth.snapshot();
        ImGui::BeginDisabled(state.busy);
        word(59, 324, "License key", IM_COL32(178, 178, 183, 255), 12, app.graphics.medium);
        at(58, 349);
        ImGui::SetNextItemWidth(334 * scale);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, p(14, 14));
        bool submit =
            ImGui::InputTextWithHint("##license-key", "Enter your license key", key, sizeof(key),
                                     ImGuiInputTextFlags_EnterReturnsTrue |
                                         (reveal ? ImGuiInputTextFlags_None : ImGuiInputTextFlags_Password));
        ImGui::PopStyleVar();
        if (button(reveal ? "Hide" : "Show", 400, 349, 60, 47))
            reveal = !reveal;
        at(58, 414);
        if (ImGui::Checkbox("Remember this license on this PC", &prefs.remember))
            savePrefs();
        at(58, 447);
        if (ImGui::SmallButton(twoFactor ? "Hide 2FA field" : "Use a two-factor code"))
            twoFactor = !twoFactor;
        float by = 482;
        if (twoFactor) {
            at(58, 477);
            ImGui::SetNextItemWidth(400 * scale);
            ImGui::InputTextWithHint("##otp", "Two-factor code", otp, sizeof(otp),
                                     ImGuiInputTextFlags_CharsDecimal);
            by = 526;
        }
        bool click = button(state.busy ? "Signing in..." : "Sign in", 58, by, 402, 46, true);
        ImGui::EndDisabled();
        if ((click || submit) && !state.busy) {
            localError.clear();
            if (demo)
                localError = "Preview mode does not validate licenses. Use Preview library below.";
            else if (!key[0])
                localError = "Enter your license key to continue.";
            else
                auth.login(key, otp, prefs.remember);
        }
        float below = by + 63;
        if (state.busy) {
            spinner(d, p(68, below + 7), 6 * scale);
            word(83, below, state.stage, muted, 11.5f);
        } else if (!localError.empty() || !state.message.empty())
            wrapped(59, below, 402, !localError.empty() ? localError : state.message,
                    IM_COL32(205, 205, 211, 255), 11.5f);
        else
            word(59, below, "Saved keys are encrypted for this Windows user.", muted, 11);
        if (demo) {
            if (button("Preview library", 58, 616, 195, 34)) {
                localError.clear();
                page = Page::Library;
            }
        } else if (!state.initialized || !state.errorCode.empty()) {
            ImGui::BeginDisabled(state.busy);
            if (button("Reconnect", 58, 616, 130, 34)) {
                localError.clear();
                auth.initialize(prefs.remember);
            }
            ImGui::EndDisabled();
        }
        if (button("Need help?", 330, 616, 130, 34))
            helpRequested = true;
        word(717, 600, "DESIGNED TO BE", IM_COL32(163, 163, 171, 255), 10, app.graphics.medium);
        word(716, 621, "Uncomplicated.", white, 22, app.graphics.medium);
        footer();
    }
    void granted() {
        auto *d = ImGui::GetWindowDrawList();
        header(false);
        app.graphics.drawLogo(d, p(400, 173), 180 * scale, prefs.animate, 90);
        d->AddCircle(p(490, 410), 28 * scale, white, 48, 1.4f * scale);
        d->AddLine(p(477, 410), p(487, 420), white, 2 * scale);
        d->AddLine(p(487, 420), p(505, 398), white, 2 * scale);
        word(349, 462, "Access granted.", white, 32, app.graphics.title);
        word(342, 512, "Your Counter-Strike 2 license is verified.", muted, 14);
        footer();
    }
    bool matches(const GameDefinition &game) {
        std::string name = game.title, query = search;
        for (char &c : name)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        for (char &c : query)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return name.find(query) != std::string::npos;
    }
    void hero() {
        auto *d = ImGui::GetWindowDrawList();
        app.graphics.drawCover(d, 0, p(24, 199), p(956, 409), 255, 11 * scale);
        d->AddRectFilledMultiColor(p(24, 199), p(956, 409), IM_COL32(8, 8, 9, 200), IM_COL32(8, 8, 9, 20),
                                   IM_COL32(8, 8, 9, 110), IM_COL32(8, 8, 9, 245));
        d->AddRect(p(24, 199), p(956, 409), IM_COL32(69, 69, 74, 255), 11 * scale);
        pill(d, 44, 219, 140, demo ? "MODULE PREVIEW" : "AVAILABLE NOW");
        word(43, 264, "Counter-Strike 2", white, 31, app.graphics.title);
        word(44, 311, "External client / Configs / Live diagnostics", IM_COL32(184, 184, 190, 255), 12);
        word(44, 365, "SUBSCRIPTION", muted, 9.5f, app.graphics.medium);
        auto state = auth.snapshot();
        word(154, 363, demo ? "Preview only" : expiryText(state.expiry), white, 11.5f);
        word(440, 365, "CLIENT", muted, 9.5f, app.graphics.medium);
        word(491, 363, "3.0", white, 11.5f);
        if (button(demo ? "Open UI preview" : "Launch", 765, 337, 165, 46, true))
            openModule();
    }
    void gameCard(size_t index, float width, float height) {
        const auto &game = catalog[index];
        auto *d = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetCursorScreenPos(), end{pos.x + width, pos.y + height};
        ImGui::PushID(game.id);
        bool clicked = ImGui::InvisibleButton("card", {width, height});
        bool hover = ImGui::IsItemHovered();
        app.graphics.drawCover(d, index, pos, end, hover ? 245 : 200, 9 * scale);
        d->AddRectFilledMultiColor(pos, end, IM_COL32(8, 8, 10, 140), IM_COL32(8, 8, 10, 30),
                                   IM_COL32(8, 8, 10, 165), IM_COL32(8, 8, 10, 242));
        d->AddRect(pos, end, hover ? IM_COL32(99, 99, 105, 255) : IM_COL32(50, 50, 55, 255), 9 * scale);
        d->AddRectFilled({pos.x + 18 * scale, pos.y + 17 * scale}, {pos.x + 116 * scale, pos.y + 39 * scale},
                         IM_COL32(19, 19, 21, 225), 11 * scale);
        d->AddText(app.graphics.small, 10 * scale, {pos.x + 28 * scale, pos.y + 22 * scale},
                   IM_COL32(184, 184, 190, 255), "COMING SOON");
        d->AddText(app.graphics.medium, 19 * scale, {pos.x + 18 * scale, pos.y + height - 61 * scale}, white,
                   game.title);
        d->AddText(app.graphics.small, 10 * scale, {pos.x + 19 * scale, pos.y + height - 31 * scale}, muted,
                   game.category);
        d->AddCircle({end.x - 28 * scale, pos.y + 28 * scale}, 11 * scale, IM_COL32(143, 143, 150, 180), 24,
                     scale);
        d->AddLine({end.x - 31 * scale, pos.y + 24 * scale}, {end.x - 25 * scale, pos.y + 28 * scale}, muted,
                   scale);
        d->AddLine({end.x - 25 * scale, pos.y + 28 * scale}, {end.x - 31 * scale, pos.y + 32 * scale}, muted,
                   scale);
        if (hover)
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        if (clicked)
            detailGame = static_cast<int>(index);
        ImGui::PopID();
    }
    void library() {
        header(true);
        word(24, 103, "Your library.", white, 32, app.graphics.title);
        word(25, 150, "One place. Ready when you are.", muted, 12.5f);
        at(691, 113);
        ImGui::SetNextItemWidth(265 * scale);
        ImGui::InputTextWithHint("##search", "Search your games", search, sizeof(search));
        const char *labels[] = {"All games  8", "Available  1", "Coming soon  7"};
        for (int i = 0; i < 3; ++i) {
            if (button(labels[i], 24 + 143.f * i, 169, 133, 27, filter == i))
                filter = i;
        }
        bool showHero = filter != 2 && matches(catalog[0]);
        if (showHero)
            hero();
        float top = showHero ? 430.f : 211.f;
        at(24, top);
        ImGui::BeginChild("##games", p(934, 676 - top), ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground);
        const float w = 450 * scale, h = 171 * scale;
        int shown = 0;
        if (filter != 1)
            for (size_t i = 1; i < catalog.size(); ++i)
                if (matches(catalog[i])) {
                    if (shown % 2)
                        ImGui::SameLine(0, 18 * scale);
                    gameCard(i, w, h);
                    ++shown;
                    if (shown % 2 == 0)
                        ImGui::Dummy(p(1, 6));
                }
        if (!shown && !showHero)
            ImGui::TextDisabled("No games match this search.");
        else if (filter == 1)
            ImGui::TextDisabled("More modules are on the way. Follow Discord for announcements.");
        ImGui::EndChild();
        footer();
    }
    void settings() {
        if (settingsRequested) {
            ImGui::OpenPopup("Workspace settings");
            settingsRequested = false;
        }
        ImGui::SetNextWindowSize(p(632, 450), ImGuiCond_Always);
        ImGui::SetNextWindowPos(p(490, 370), ImGuiCond_Always, {.5f, .5f});
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, p(24, 20));
        if (ImGui::BeginPopupModal("Workspace settings", nullptr,
                                   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings)) {
            ImGui::TextDisabled("APPEARANCE");
            if (ImGui::Checkbox("Animate logo and background", &prefs.animate))
                savePrefs();
            constexpr int rates[] = {60, 90, 120, 144};
            int choice = 0;
            for (int i = 0; i < 4; ++i)
                if (prefs.uiHz >= rates[i])
                    choice = i;
            ImGui::SetNextItemWidth(200 * scale);
            if (ImGui::Combo("Interface refresh target", &choice,
                             "60 Hz\0"
                             "90 Hz\0"
                             "120 Hz\0"
                             "144 Hz\0")) {
                prefs.uiHz = rates[choice];
                savePrefs();
            }
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextDisabled("PRIVACY & SIGN-IN");
            if (ImGui::Checkbox("Capture privacy (best effort)", &prefs.capturePrivacy))
                savePrefs();
            ImGui::TextWrapped("%s", app.capturePrivacy.status().c_str());
            ImGui::TextDisabled("Windows capture exclusion is not a universal streamproof guarantee.");
            if (ImGui::Checkbox("Remember future sign-ins on this PC", &prefs.remember))
                savePrefs();
            ImGui::TextDisabled("Saved licenses use Windows user-bound DPAPI encryption.");
            ImGui::Spacing();
            ImGui::BeginDisabled(demo || auth.snapshot().busy);
            if (ImGui::Button("Forget saved license and sign out", p(365, 34))) {
                try {
                    auth.forget();
                    app.stopOutput();
                    page = Page::License;
                    localError.clear();
                    auth.initialize(false);
                    ImGui::CloseCurrentPopup();
                } catch (const std::exception &) {
                    app.notify("The saved license could not be removed.");
                }
            }
            ImGui::EndDisabled();
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextDisabled("CS2 config files: %s", utf8((app.directory / "configs").wstring()).c_str());
            if (ImGui::Button("Open data folder", p(192, 33)))
                app.open(app.directory);
            ImGui::SameLine();
            if (ImGui::Button("Close", p(104, 33)))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
        ImGui::PopStyleVar();
    }
    void dialogs() {
        if (helpRequested) {
            ImGui::OpenPopup("Sign-in help");
            helpRequested = false;
        }
        ImGui::SetNextWindowSize(p(570, 330), ImGuiCond_Always);
        ImGui::SetNextWindowPos(p(490, 370), ImGuiCond_Always, {.5f, .5f});
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, p(24, 20));
        if (ImGui::BeginPopupModal("Sign-in help", nullptr, ImGuiWindowFlags_NoResize)) {
            ImGui::TextWrapped("Use a customer license created for the Counter Strike 2 KeyAuth application. "
                               "Do not enter the application secret.");
            ImGui::Spacing();
            ImGui::TextWrapped("Licenses are checked online. Expired keys, a different device binding, "
                               "missing 2FA, and service errors do not grant access.");
            ImGui::Spacing();
            ImGui::TextWrapped("Remembered keys stay encrypted for this Windows user and are revalidated on "
                               "the next start. Other games are not released yet.");
            ImGui::Spacing();
            if (ImGui::Button("Community / support", p(218, 35)))
                app.openUrl(L"https://discord.gg/v6xcore");
            ImGui::SameLine();
            if (ImGui::Button("Close", p(110, 35)))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
        ImGui::PopStyleVar();
        if (detailGame >= 0)
            ImGui::OpenPopup("Coming soon");
        ImGui::SetNextWindowSize(p(540, 288), ImGuiCond_Always);
        ImGui::SetNextWindowPos(p(490, 370), ImGuiCond_Always, {.5f, .5f});
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, p(24, 22));
        if (ImGui::BeginPopupModal("Coming soon", nullptr, ImGuiWindowFlags_NoResize)) {
            if (detailGame >= 0) {
                const auto &game = catalog[static_cast<size_t>(detailGame)];
                ImGui::Text("%s", game.title);
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::TextWrapped("The library artwork is ready. This module and its separate KeyAuth "
                                   "application are not configured or available yet.");
                ImGui::Spacing();
                ImGui::TextDisabled("No release date has been announced.");
            }
            ImGui::Spacing();
            if (ImGui::Button("Open Discord", p(210, 35)))
                app.openUrl(L"https://discord.gg/v6xcore");
            ImGui::SameLine();
            if (ImGui::Button("Got it", p(112, 35))) {
                detailGame = -1;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        ImGui::PopStyleVar();
    }
    void render() {
        auto &io = ImGui::GetIO();
        scale = std::min(io.DisplaySize.x / 980.f, io.DisplaySize.y / 740.f);
        if (scale <= 0)
            return;
        if (page == Page::Module) {
            core.render();
            if (core.wantLibrary) {
                core.wantLibrary = false;
                app.stopOutput();
                page = Page::Library;
            }
            if (core.wantSettings) {
                core.wantSettings = false;
                settingsRequested = true;
            }
            theme();
            settings();
            dialogs();
            return;
        }
        theme();
        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("v6xCore Launcher", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar |
                         ImGuiWindowFlags_NoScrollWithMouse);
        if (page == Page::Boot)
            boot();
        else if (page == Page::License)
            license();
        else if (page == Page::Granted)
            granted();
        else
            library();
        ImGui::End();
        settings();
        dialogs();
    }
};
} // namespace v6x
