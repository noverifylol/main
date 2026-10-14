#pragma once
#include "app.hpp"
#include <cfloat>
#include <unordered_map>

namespace v6x {
struct UI {
    App &app;
    float scale = 1;
    int tab = 4;
    bool previewTeam = false;
    bool wantLibrary = false, wantSettings = false;
    std::string selected = "Balanced", pendingConfig;
    bool deleting = false;
    char configName[64]{};
    std::unordered_map<ImGuiID, float> animations;
    ImU32 bg = IM_COL32(14, 14, 15, 255), panel = IM_COL32(21, 21, 23, 255),
          border = IM_COL32(42, 42, 44, 255);
    ImU32 textColor = IM_COL32(228, 228, 236, 255), muted = IM_COL32(131, 132, 147, 255),
          quiet = IM_COL32(91, 93, 108, 255);
    explicit UI(App &a) : app(a) {}
    ImVec2 point(float x, float y) const {
        return {x * scale, y * scale};
    }
    void at(float x, float y) {
        ImGui::SetCursorPos(point(x, y));
    }
    ImU32 accent(int alpha = 255) const {
        const auto &a = app.config.accent;
        return IM_COL32(static_cast<int>(a[0] * 255), static_cast<int>(a[1] * 255),
                        static_cast<int>(a[2] * 255), alpha);
    }
    ImU32 col(const std::array<float, 3> &a, int alpha = 255) const {
        return IM_COL32(static_cast<int>(a[0] * 255), static_cast<int>(a[1] * 255),
                        static_cast<int>(a[2] * 255), alpha);
    }
    void drawText(ImDrawList *d, float x, float y, const char *value, ImU32 color, ImFont *font = nullptr,
                  float size = 14) {
        d->AddText(font ? font : app.graphics.body, size * scale, point(x, y), color, value);
    }
    void theme() {
        ImGuiStyle st;
        st.WindowPadding = {0, 0};
        st.FramePadding = {10, 7};
        st.ItemSpacing = {8, 6};
        st.WindowRounding = 10;
        st.ChildRounding = 8;
        st.PopupRounding = 8;
        st.FrameRounding = 5;
        st.GrabRounding = 5;
        st.ScrollbarRounding = 8;
        st.WindowBorderSize = 0;
        st.ChildBorderSize = 0;
        st.PopupBorderSize = 1;
        st.FrameBorderSize = 1;
        st.ScrollbarSize = 5;
        auto &c = st.Colors;
        c[ImGuiCol_Text] = ImGui::ColorConvertU32ToFloat4(textColor);
        c[ImGuiCol_TextDisabled] = ImGui::ColorConvertU32ToFloat4(muted);
        c[ImGuiCol_WindowBg] = ImGui::ColorConvertU32ToFloat4(bg);
        c[ImGuiCol_ChildBg] = ImVec4(0, 0, 0, 0);
        c[ImGuiCol_PopupBg] = ImVec4(.085f, .085f, .11f, 1);
        c[ImGuiCol_Border] = ImGui::ColorConvertU32ToFloat4(border);
        c[ImGuiCol_FrameBg] = ImVec4(.105f, .105f, .135f, 1);
        c[ImGuiCol_FrameBgHovered] = ImVec4(.14f, .14f, .17f, 1);
        c[ImGuiCol_FrameBgActive] = ImVec4(.16f, .16f, .20f, 1);
        c[ImGuiCol_Button] = ImVec4(.12f, .12f, .15f, 1);
        c[ImGuiCol_ButtonHovered] = ImVec4(.17f, .17f, .21f, 1);
        c[ImGuiCol_ButtonActive] = ImVec4(.20f, .20f, .24f, 1);
        c[ImGuiCol_CheckMark] = ImGui::ColorConvertU32ToFloat4(accent());
        c[ImGuiCol_SliderGrab] = c[ImGuiCol_CheckMark];
        c[ImGuiCol_SliderGrabActive] = c[ImGuiCol_CheckMark];
        c[ImGuiCol_NavHighlight] = ImGui::ColorConvertU32ToFloat4(accent(190));
        c[ImGuiCol_Header] = ImGui::ColorConvertU32ToFloat4(accent(38));
        c[ImGuiCol_HeaderHovered] = ImGui::ColorConvertU32ToFloat4(accent(24));
        c[ImGuiCol_HeaderActive] = c[ImGuiCol_Header];
        c[ImGuiCol_Separator] = c[ImGuiCol_Border];
        c[ImGuiCol_ResizeGrip] = ImVec4(0, 0, 0, 0);
        c[ImGuiCol_TitleBg] = ImVec4(.095f, .095f, .12f, 1);
        c[ImGuiCol_TitleBgActive] = c[ImGuiCol_TitleBg];
        c[ImGuiCol_TitleBgCollapsed] = c[ImGuiCol_TitleBg];
        c[ImGuiCol_ModalWindowDimBg] = ImVec4(0, 0, 0, .65f);
        c[ImGuiCol_NavWindowingDimBg] = ImVec4(0, 0, 0, .65f);
        c[ImGuiCol_TextSelectedBg] = ImGui::ColorConvertU32ToFloat4(accent(65));
        st.ScaleAllSizes(scale);
        ImGui::GetStyle() = st;
        ImGui::GetIO().FontGlobalScale = scale;
    }
    void card(float x, float y, float w, float h, const char *title, const char *detail = nullptr) {
        auto *d = ImGui::GetWindowDrawList();
        d->AddRectFilled(point(x, y), point(x + w, y + h), panel, 8 * scale);
        d->AddRect(point(x, y), point(x + w, y + h), border, 8 * scale);
        drawText(d, x + 18, y + 14, title, textColor, app.graphics.medium, 13);
        if (detail) {
            const auto width = app.graphics.small->CalcTextSizeA(11 * scale, FLT_MAX, 0, detail).x;
            d->AddText(app.graphics.small, 11 * scale, point(x + w - 18 - width / scale, y + 15), muted,
                       detail);
        }
        d->AddLine(point(x + 1, y + 42), point(x + w - 1, y + 42), border);
        at(x + 18, y + 51);
        ImGui::BeginChild(title, point(w - 36, h - 61), ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground);
    }
    void end() {
        ImGui::EndChild();
    }
    bool toggle(const char *label, bool &value, const char *hint = nullptr) {
        auto pos = ImGui::GetCursorScreenPos();
        float width = ImGui::GetContentRegionAvail().x;
        const float height = 32 * scale;
        ImGuiID id = ImGui::GetID(label);
        bool changed = ImGui::InvisibleButton(label, {width, height}, ImGuiButtonFlags_EnableNav);
        if (changed) {
            value = !value;
            app.change();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            if (hint)
                ImGui::SetTooltip("%s", hint);
        }
        float &t = animations[id];
        float target = value ? 1.f : 0.f;
        t += (target - t) * std::min(1.f, ImGui::GetIO().DeltaTime * 16);
        auto *d = ImGui::GetWindowDrawList();
        d->AddText(app.graphics.body, 14 * scale, {pos.x, pos.y + 5 * scale}, textColor, label);
        float x = pos.x + width - 32 * scale, y = pos.y + 7 * scale;
        ImVec4 off(.16f, .16f, .20f, 1), on = ImGui::ColorConvertU32ToFloat4(accent());
        ImVec4 mix(off.x + (on.x - off.x) * t, off.y + (on.y - off.y) * t, off.z + (on.z - off.z) * t, 1);
        d->AddRectFilled({x, y}, {x + 32 * scale, y + 18 * scale}, ImGui::ColorConvertFloat4ToU32(mix),
                         10 * scale);
        d->AddCircleFilled({x + (9 + 14 * t) * scale, y + 9 * scale}, 5.5f * scale,
                           IM_COL32(245, 245, 250, 255));
        ImGui::SetCursorScreenPos({pos.x, pos.y + height});
        return changed;
    }
    bool slider(const char *label, float &value, float minimum, float maximum, const char *format) {
        ImGui::PushID(label);
        ImGui::TextUnformatted(label);
        ImGui::SetNextItemWidth(-FLT_MIN);
        bool result =
            ImGui::SliderFloat("##value", &value, minimum, maximum, format, ImGuiSliderFlags_AlwaysClamp);
        if (result)
            app.change();
        ImGui::PopID();
        return result;
    }
    bool combo(const char *label, int &value, const char *choices) {
        ImGui::PushID(label);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label);
        float w = ImGui::GetWindowContentRegionMax().x;
        ImGui::SameLine(w - 185 * scale);
        ImGui::SetNextItemWidth(185 * scale);
        bool changed = ImGui::Combo("##value", &value, choices);
        if (changed)
            app.change();
        ImGui::PopID();
        return changed;
    }
    bool colorRow(const char *label, std::array<float, 3> &value) {
        ImGui::PushID(label);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label);
        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 30 * scale);
        bool result = ImGui::ColorEdit3("##color", value.data(),
                                        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel |
                                            ImGuiColorEditFlags_NoSidePreview);
        if (result)
            app.change();
        ImGui::PopID();
        return result;
    }
    bool button(const char *label, float width = 0, bool primary = false) {
        if (primary) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::ColorConvertU32ToFloat4(accent()));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::ColorConvertU32ToFloat4(accent(220)));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(.07f, .06f, .08f, 1));
        }
        bool result = ImGui::Button(label, {width > 0 ? width * scale : width, 34 * scale});
        if (primary)
            ImGui::PopStyleColor(3);
        return result;
    }
    void note(const char *value) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(muted));
        ImGui::TextWrapped("%s", value);
        ImGui::PopStyleColor();
    }
    void metric(const char *label, const std::string &value, ImU32 color = 0) {
        ImGui::TextDisabled("%s", label);
        ImGui::SameLine(155 * scale);
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(color ? color : textColor));
        ImGui::TextUnformatted(value.c_str());
        ImGui::PopStyleColor();
    }
    void icon(ImDrawList *d, int type, ImVec2 p, ImU32 c, float s = 1) {
        const float a = scale * s;
        auto q = [&](float x, float y) { return ImVec2(p.x + x * a, p.y + y * a); };
        if (type == 0) {
            d->AddCircle(q(10, 10), 6 * a, c, 24, 1.4f * a);
            for (int i = 0; i < 4; ++i) {
                float r = i * cs2::pi / 2;
                d->AddLine(q(10 + 8 * std::cos(r), 10 + 8 * std::sin(r)),
                           q(10 + 11 * std::cos(r), 10 + 11 * std::sin(r)), c, 1.4f * a);
            }
        } else if (type == 1) {
            d->AddBezierCubic(q(0, 10), q(5, 0), q(15, 0), q(20, 10), c, 1.4f * a);
            d->AddBezierCubic(q(0, 10), q(5, 20), q(15, 20), q(20, 10), c, 1.4f * a);
            d->AddCircle(q(10, 10), 3 * a, c, 18, 1.4f * a);
        } else if (type == 2) {
            d->AddRect(q(1, 1), q(15, 14), c, 2 * a, 0, 1.4f * a);
            d->AddLine(q(6, 18), q(19, 18), c, 1.4f * a);
            d->AddLine(q(19, 6), q(19, 18), c, 1.4f * a);
        } else if (type == 3) {
            d->AddLine(q(1, 4), q(8, 4), c, 1.4f * a);
            d->AddLine(q(8, 4), q(11, 7), c, 1.4f * a);
            d->AddLine(q(11, 7), q(20, 7), c, 1.4f * a);
            d->AddLine(q(20, 7), q(20, 17), c, 1.4f * a);
            d->AddLine(q(20, 17), q(1, 17), c, 1.4f * a);
            d->AddLine(q(1, 17), q(1, 4), c, 1.4f * a);
        } else {
            for (int i = 0; i < 3; ++i) {
                float y = 4.f + 6 * i;
                d->AddLine(q(1, y), q(20, y), c, 1.3f * a);
                d->AddCircleFilled(q(i % 2 ? 14.f : 7.f, y), 2.5f * a, c);
            }
        }
    }
    void drawPreview() {
        auto *d = ImGui::GetWindowDrawList();
        auto p = ImGui::GetCursorScreenPos();
        float w = ImGui::GetContentRegionAvail().x, h = ImGui::GetContentRegionAvail().y;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {9 * scale, 4 * scale});
        if (ImGui::SmallButton(previewTeam ? "Team-side player  /  switch" : "Opponent  /  switch"))
            previewTeam = !previewTeam;
        ImGui::PopStyleVar();
        d->AddText(app.graphics.small, 11 * scale, {p.x + w - 117 * scale, p.y + 4 * scale}, muted,
                   "SIMULATED DATA");
        float center = p.x + w * .50f, top = p.y + 49 * scale, bottom = p.y + h - 26 * scale;
        float bodyHeight = bottom - top;
        ImU32 figure = IM_COL32(68, 69, 84, 190);
        d->AddCircleFilled({center, top + 14 * scale}, 12 * scale, IM_COL32(37, 38, 48, 255));
        d->AddCircle({center, top + 14 * scale}, 12 * scale, figure, 30, 1.2f * scale);
        float shoulders = top + 34 * scale, hip = top + bodyHeight * .58f, knee = top + bodyHeight * .79f;
        d->AddQuadFilled({center - 25 * scale, shoulders}, {center + 25 * scale, shoulders},
                         {center + 18 * scale, hip}, {center - 18 * scale, hip}, IM_COL32(32, 33, 42, 255));
        d->AddQuad({center - 25 * scale, shoulders}, {center + 25 * scale, shoulders},
                   {center + 18 * scale, hip}, {center - 18 * scale, hip}, figure, 1.2f * scale);
        for (int sign : {-1, 1}) {
            d->AddLine({center + sign * 28 * scale, shoulders + 4 * scale},
                       {center + sign * 38 * scale, hip - 8 * scale}, figure, 7 * scale);
            d->AddLine({center + sign * 10 * scale, hip}, {center + sign * 15 * scale, knee}, figure,
                       10 * scale);
            d->AddLine({center + sign * 15 * scale, knee}, {center + sign * 18 * scale, bottom}, figure,
                       9 * scale);
        }
        const auto &c = app.config;
        bool visible = c.esp && (!previewTeam || c.freeForAll || c.teammates) && c.maxDistance >= 12.4f;
        if (visible) {
            ImU32 cc = col(previewTeam && !c.freeForAll ? c.allyColor : c.enemyColor);
            float x = center - 51 * scale, y = top - 7 * scale, ww = 102 * scale, hh = bottom - y + 5 * scale;
            if (c.snaplines)
                d->AddLine({p.x + w / 2, c.lineOrigin == 0   ? p.y + 29 * scale
                                         : c.lineOrigin == 1 ? p.y + h / 2
                                                             : p.y + h},
                           {center, bottom}, cc, scale);
            if (c.boxes) {
                if (c.boxStyle == 1) {
                    for (int xx : {0, 1})
                        for (int yy : {0, 1}) {
                            ImVec2 z{x + xx * ww, y + yy * hh};
                            d->AddLine(z, {z.x + (xx ? -18 : 18) * scale, z.y}, cc, c.thickness * scale);
                            d->AddLine(z, {z.x, z.y + (yy ? -23 : 23) * scale}, cc, c.thickness * scale);
                        }
                } else
                    d->AddRect({x, y}, {x + ww, y + hh}, cc, c.boxStyle == 2 ? 7 * scale : 0, 0,
                               c.thickness * scale);
            }
            if (c.names)
                d->AddText(app.graphics.medium, 12 * scale, {center - 35 * scale, y - 20 * scale}, cc,
                           previewTeam ? "Teammate" : "Player 01");
            if (c.health) {
                d->AddRectFilled({x - 8 * scale, y}, {x - 5 * scale, y + hh}, IM_COL32(39, 41, 45, 255));
                d->AddRectFilled({x - 8 * scale, y + hh * .15f}, {x - 5 * scale, y + hh},
                                 IM_COL32(119, 226, 173, 255));
            }
            std::string label;
            if (c.healthText)
                label = "85 HP";
            if (c.distance) {
                if (!label.empty())
                    label += "  /  ";
                label += "12.4 m";
            }
            float tw = app.graphics.small->CalcTextSizeA(11 * scale, FLT_MAX, 0, label.c_str()).x;
            d->AddText(app.graphics.small, 11 * scale, {center - tw / 2, bottom + 10 * scale}, textColor,
                       label.c_str());
            if (c.headMarker)
                d->AddCircle({center, top + 14 * scale}, 4 * scale, cc, 18, scale);
        } else
            d->AddText(app.graphics.small, 11 * scale, {p.x + 3 * scale, p.y + h - 15 * scale}, muted,
                       "Hidden by the current filters");
        ImGui::Dummy({w, h - 27 * scale});
    }
    void visuals() {
        auto &c = app.config;
        card(24, 174, 446, 324, "Player visuals", "F1 / MASTER");
        toggle("Enable ESP", c.esp);
        toggle("Player boxes", c.boxes);
        toggle("Player names", c.names);
        toggle("Health bar", c.health);
        toggle("Health value", c.healthText);
        toggle("Distance", c.distance);
        toggle("Head marker", c.headMarker, "Approximate head height, not an exact hitbox.");
        toggle("Outline", c.outlines);
        end();
        card(24, 514, 446, 144, "Filters", c.freeForAll ? "FREE-FOR-ALL" : "TEAM MATCH");
        ImGui::BeginDisabled(c.freeForAll);
        toggle("Show teammates", c.teammates,
               "In free-for-all, every other player is treated as an opponent.");
        ImGui::EndDisabled();
        slider("Maximum distance", c.maxDistance, 5, 500, "%.0f m");
        end();
        card(494, 174, 462, 316, "Visual preview", "LOCAL PREVIEW");
        drawPreview();
        end();
        card(494, 506, 462, 152, "Box style");
        combo("Appearance", c.boxStyle, "Full box\0Corners\0Rounded\0");
        slider("Line width", c.thickness, 1, 4, "%.1f px");
        end();
    }
    void aimTab() {
        auto &c = app.config;
        card(24, 174, 446, 288, "Aim control", "F2 / ENABLE");
        if (toggle("Enable aim control", c.aim, "Disabled on startup and after loading a config."))
            app.aim.reset();
        toggle("Radar-known targets only", c.spottedOnly,
               "Spotted is not a visibility test. Targets may be behind walls.");
        int key = c.aimKey == VK_RBUTTON    ? 1
                  : c.aimKey == VK_XBUTTON1 ? 2
                  : c.aimKey == VK_XBUTTON2 ? 3
                  : c.aimKey == VK_LSHIFT   ? 4
                                            : 0;
        if (combo("Hold key", key, "ALT\0Right mouse\0MOUSE4\0MOUSE5\0Left shift\0")) {
            constexpr int codes[] = {VK_MENU, VK_RBUTTON, VK_XBUTTON1, VK_XBUTTON2, VK_LSHIFT};
            c.aimKey = codes[key];
            app.aim.reset();
        }
        slider("FOV half-angle", c.fov, 1, 15, "%.1f deg");
        slider("Smoothing", c.smoothing, 1, 20, "%.1f / 20");
        end();
        card(24, 478, 446, 180, "Player relations");
        int mode = c.freeForAll ? 1 : 0;
        if (combo("Match mode", mode, "Team matches\0Free-for-all\0")) {
            c.freeForAll = mode == 1;
            app.aim.reset();
        }
        ImGui::BeginDisabled(c.freeForAll);
        if (toggle("Target teammates", c.targetTeammates))
            app.aim.reset();
        ImGui::EndDisabled();
        note(c.freeForAll ? "FFA: all other players are opponents. Self and immune players stay excluded."
                          : "Team targeting is opt-in. Game rules are not changed.");
        end();
        card(494, 174, 462, 290, "FOV preview", "SIMULATION");
        {
            auto p = ImGui::GetCursorScreenPos();
            auto *d = ImGui::GetWindowDrawList();
            float w = ImGui::GetContentRegionAvail().x, h = 190 * scale;
            ImVec2 center{p.x + w / 2, p.y + h / 2};
            d->AddLine({p.x, center.y}, {p.x + w, center.y}, border);
            d->AddLine({center.x, p.y}, {center.x, p.y + h}, border);
            for (int i = 1; i <= 3; ++i)
                d->AddCircle(center, i * 28 * scale, IM_COL32(39, 40, 50, 255), 64, scale);
            float r = (22 + c.fov * 4) * scale;
            d->AddCircleFilled(center, r, accent(9), 80);
            d->AddCircle(center, r, accent(180), 80, 1.3f * scale);
            for (int i = 0; i < 5; ++i) {
                float a = i * 1.3f + .5f;
                float rr = (i % 2 ? 58.f : 31.f) * scale;
                d->AddCircleFilled({center.x + std::cos(a) * rr, center.y + std::sin(a) * rr}, 3 * scale,
                                   rr < r ? accent() : quiet);
            }
            d->AddCircleFilled(center, 2 * scale, textColor);
            ImGui::Dummy({w, h});
            note("Selection-angle illustration. Not a live target.");
        }
        end();
        card(494, 480, 462, 178, "Automatic input scaling");
        metric("Sensitivity",
               app.frame.valid ? utf8(fixed(app.frame.sensitivity, 2)) : "Waiting for live data");
        metric("Scoped scaling", "From game data");
        note("Uses live sensitivity and scoped scaling. Designed for the default CS2 input settings.");
        end();
    }
    void overlayTab() {
        auto &c = app.config;
        card(24, 174, 446, 318, "Overlay elements");
        toggle("FOV ring", c.fovRing);
        toggle("Snaplines", c.snaplines);
        toggle("Crosshair", c.crosshair);
        toggle("v6xCore watermark", c.watermark);
        toggle("Show measured sampling rate", c.showFps);
        slider("Text size", c.textSize, 10, 22, "%.0f px");
        end();
        card(24, 508, 446, 150, "Rendering");
        slider("Overlay opacity", c.opacity, .2f, 1, "%.2f");
        constexpr int rates[] = {30, 60, 120, 165, 240, 360};
        int rate = 0;
        for (int i = 0; i < 6; ++i)
            if (c.overlayFps >= rates[i])
                rate = i;
        if (combo("Render target", rate,
                  "30 Hz\0"
                  "60 Hz\0"
                  "120 Hz\0"
                  "165 Hz\0"
                  "240 Hz\0"
                  "360 Hz\0"))
            c.overlayFps = rates[rate];
        end();
        card(494, 174, 462, 280, "Colors", "CUSTOMIZABLE");
        colorRow("UI accent", c.accent);
        colorRow("Opponents", c.enemyColor);
        colorRow("Teammates", c.allyColor);
        colorRow("Current target", c.targetColor);
        ImGui::Spacing();
        if (ImGui::SmallButton("Coral")) {
            c.accent = {1, .45f, .47f};
            app.change();
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Ice")) {
            c.accent = {.51f, .75f, 1};
            app.change();
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Mint")) {
            c.accent = {.48f, .88f, .73f};
            app.change();
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Silver")) {
            c.accent = {.77f, .79f, .87f};
            app.change();
        }
        end();
        card(494, 470, 462, 188, "Appearance");
        combo("Line origin", c.lineOrigin, "Top\0Center\0Bottom\0");
        slider("Crosshair size", c.crosshairSize, 2, 20, "%.0f px");
        toggle("Animate logo", c.animation);
        end();
    }
    void configTab() {
        card(24, 174, 446, 484, "Local configs", "INI / LOCAL");
        if (app.configs.empty())
            note("No configs yet. Save a new config on the right.");
        for (const auto &name : app.configs) {
            ImGui::PushID(name.c_str());
            if (ImGui::Selectable(name.c_str(), selected == name, ImGuiSelectableFlags_AllowDoubleClick,
                                  {0, 40 * scale})) {
                selected = name;
                if (ImGui::IsMouseDoubleClicked(0))
                    app.loadConfig(name);
            }
            ImGui::PopID();
        }
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::BeginDisabled(selected.empty() || std::find(app.configs.begin(), app.configs.end(),
                                                           selected) == app.configs.end());
        if (button("Load config", 180, true))
            app.loadConfig(selected);
        ImGui::SameLine();
        if (button("Overwrite", 180)) {
            pendingConfig = selected;
            deleting = false;
            ImGui::OpenPopup("Confirm config action");
        }
        if (button("Delete config", 180)) {
            pendingConfig = selected;
            deleting = true;
            ImGui::OpenPopup("Confirm config action");
        }
        ImGui::EndDisabled();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, point(16, 14));
        if (ImGui::BeginPopupModal("Confirm config action", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("%s: %s?", deleting ? "Delete" : "Overwrite", pendingConfig.c_str());
            ImGui::Spacing();
            if (button("Confirm", 140, true)) {
                if (deleting) {
                    app.removeConfig(pendingConfig);
                    selected = app.configs.empty() ? "" : app.configs.front();
                } else
                    app.saveConfig(pendingConfig);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (button("Cancel", 140))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
        ImGui::PopStyleVar();
        end();
        card(494, 174, 462, 238, "New config");
        note("Save the current visual, color, and input settings as a config.");
        ImGui::Spacing();
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::InputTextWithHint("##config-name", "Config name", configName, sizeof(configName));
        if (button("Save new config", 0, true)) {
            auto safe = cs2::safeConfigName(configName);
            if (safe && std::filesystem::exists(app.directory / "configs" / (*safe + ".ini")))
                app.notify("That name already exists. Use Overwrite on the left.");
            else if (app.saveConfig(configName)) {
                selected = app.selectedConfig;
                configName[0] = 0;
            }
        }
        note("A-Z, 0-9, spaces, - and _. Maximum 40 characters.");
        end();
        card(494, 428, 462, 230, "Management");
        metric("Active config", app.selectedConfig);
        metric("Settings", app.dirty ? "Unsaved changes" : "Saved");
        if (button("Open config folder", 205))
            app.open(app.directory / "configs");
        if (button("Reset settings", 300))
            app.resetSettings();
        note("Configs never enable aim control automatically. Colors and values apply immediately.");
        end();
    }
    void systemTab() {
        card(24, 174, 446, 276, "Connection & overlay", app.testMode ? "UI PREVIEW" : "USER MODE");
        metric("CS2 detected",
               app.process.foundPid() ? "PID " + std::to_string(app.process.foundPid()) : "Not found yet");
        metric("Read access", app.process.alive() ? "Connected" : "Not connected");
        metric("Live data", app.frame.valid ? "Available" : "Unavailable");
        metric("Process name", "cs2.exe");
        ImGui::Spacing();
        note(app.status.c_str());
        ImGui::Separator();
        note(app.overlayStatus().c_str());
        end();
        card(24, 466, 446, 192, "Diagnostics");
        if (button("Export report", 120, true))
            app.diagnostics();
        ImGui::SameLine();
        if (button("Open log", 110))
            app.open(app.directory / "v6xcore.log");
        ImGui::SameLine();
        if (button("Data folder", 145))
            app.open(app.directory);
        ImGui::BeginChild("##log", {0, 78 * scale}, ImGuiChildFlags_None,
                          ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::PushFont(app.graphics.small);
        for (auto it = app.logs.rbegin(); it != app.logs.rend(); ++it) {
            ImGui::TextDisabled("%s", it->time.c_str());
            ImGui::SameLine();
            ImGui::TextUnformatted(it->message.c_str());
        }
        ImGui::PopFont();
        ImGui::EndChild();
        end();
        card(494, 174, 462, 342, "Offset data", "NATIVE UPDATER");
        metric("File validation", app.offsets.valid ? "Loaded successfully" : "Load error",
               app.offsets.valid ? IM_COL32(122, 221, 169, 255) : accent());
        metric("Build / date", app.offsets.valid ? std::to_string(app.offsets.build) + " / " +
                                                       utf8(app.offsets.sourceDate.substr(0, 10))
                                                 : "- ");
        ImGui::TextDisabled("Source");
        ImGui::PushFont(app.graphics.small);
        ImGui::TextUnformatted(utf8(app.offsets.sourceUrl).c_str());
        ImGui::TextUnformatted(utf8(app.offsets.sourceCommit).c_str());
        ImGui::PopFont();
        ImGui::Spacing();
        if (!app.offsets.valid)
            note(utf8(app.offsets.error).c_str());
        else
            note(app.restoredAtStartup ? "Missing offsets were restored from the executable."
                                       : "Local offsets loaded. This is not a live compatibility test.");
        ImGui::BeginDisabled(app.updater.busy());
        if (button("Update online", 211, true))
            app.startUpdate();
        ImGui::SameLine();
        if (button("Restore bundled data", 199))
            app.restoreOffsets();
        ImGui::EndDisabled();
        if (app.updater.busy())
            note(app.updater.stage().c_str());
        else
            note("HTTPS updates need no Python. Restore bundled data keeps a backup. F9 reloads files.");
        end();
        card(494, 532, 462, 126, "Performance", "TARGET / MEASURED");
        constexpr int rates[] = {60, 120, 165, 240, 360};
        int selectedRate = 0;
        for (int i = 0; i < 5; ++i)
            if (app.config.sampleHz >= rates[i])
                selectedRate = i;
        if (combo("Sampling target", selectedRate,
                  "60 Hz\0"
                  "120 Hz\0"
                  "165 Hz\0"
                  "240 Hz\0"
                  "360 Hz\0"))
            app.config.sampleHz = rates[selectedRate];
        ImGui::TextDisabled("Measured: %.0f Hz   |   Read: %.2f ms", app.measuredFps, app.sampleMs);
        end();
    }
    void render() {
        auto &io = ImGui::GetIO();
        scale = std::min(io.DisplaySize.x / 980.f, io.DisplaySize.y / 740.f);
        if (scale <= 0)
            return;
        theme();
        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("v6xCore Cs2 External", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar |
                         ImGuiWindowFlags_NoScrollWithMouse);
        auto *d = ImGui::GetWindowDrawList();
        d->AddRectFilled(point(0, 0), point(980, 72), IM_COL32(17, 17, 18, 255));
        d->AddLine(point(0, 72), point(980, 72), border);
        app.graphics.drawLogo(d, point(18, 8), 57 * scale, app.config.animation);
        drawText(d, 84, 16, "v6x", textColor, app.graphics.medium, 20);
        drawText(d, 119, 16, "Core", accent(), app.graphics.medium, 20);
        drawText(d, 85, 43, "Cs2 External", muted, app.graphics.small, 11.5f);
        const char *state = !app.offsets.valid       ? "OFFSET ERROR"
                            : app.testMode           ? "UI PREVIEW"
                            : app.paused             ? "PAUSED"
                            : app.frame.valid        ? "DATA READY"
                            : !app.connected         ? "DISCONNECTED"
                            : app.process.foundPid() ? "CS2 FOUND"
                                                     : "LOOKING FOR CS2";
        ImU32 stateColor = !app.offsets.valid ? accent()
                           : app.frame.valid  ? IM_COL32(122, 221, 169, 255)
                           : app.connected    ? IM_COL32(226, 185, 121, 255)
                                              : muted;
        d->AddRectFilled(point(330, 23), point(479, 49), IM_COL32(27, 27, 29, 255), 5 * scale);
        d->AddCircleFilled(point(343, 36), 3 * scale, stateColor);
        drawText(d, 354, 28, state, stateColor, app.graphics.small, 10);
        at(496, 19);
        if (button("Library", 122))
            wantLibrary = true;
        at(632, 19);
        ImGui::BeginDisabled(app.testMode || !app.offsets.valid || !app.licensed);
        if (button(app.connected ? "Disconnect" : "Connect", 135))
            app.connect();
        ImGui::EndDisabled();
        at(780, 19);
        if (button(app.paused ? "Resume" : "Pause", 78))
            app.pause();
        at(868, 20);
        if (ImGui::Button("...", point(29, 30)))
            wantSettings = true;
        at(903, 20);
        if (ImGui::Button("-", point(27, 30)))
            ShowWindow(app.window, SW_MINIMIZE);
        ImGui::SameLine(934 * scale);
        if (ImGui::Button("x", point(27, 30)))
            PostMessageW(app.window, WM_CLOSE, 0, 0);
        const char *titles[] = {"Aim control", "Visuals", "Overlay & Colors", "Configs", "System"};
        const char *descriptions[] = {"Deliberate input. Clear controls.",
                                      "Essential information. Less visual noise.",
                                      "Your look, down to the details.", "Your settings, saved and ready.",
                                      "Clear status. Traceable data."};
        drawText(d, 24, 96, titles[tab], textColor, app.graphics.heading, 24);
        drawText(d, 25, 134, descriptions[tab], muted, app.graphics.body, 12);
        std::string saved = app.dirty ? "Saving changes" : "Settings saved";
        float sw = app.graphics.small->CalcTextSizeA(11 * scale, FLT_MAX, 0, saved.c_str()).x;
        d->AddCircleFilled(point(938 - sw / scale, 144), 2.5f * scale,
                           app.dirty ? accent() : IM_COL32(112, 185, 145, 255));
        d->AddText(app.graphics.small, 11 * scale, point(947 - sw / scale, 136), muted, saved.c_str());
        switch (tab) {
        case 0:
            aimTab();
            break;
        case 1:
            visuals();
            break;
        case 2:
            overlayTab();
            break;
        case 3:
            configTab();
            break;
        case 4:
            systemTab();
            break;
        }
        d->AddRectFilled(point(0, 676), point(980, 740), IM_COL32(17, 17, 18, 255));
        d->AddLine(point(0, 676), point(980, 676), border);
        const char *names[] = {"Aim control", "Visuals", "Overlay", "Configs", "System"};
        for (int i = 0; i < 5; ++i) {
            float x = 19.f + i * 148;
            at(x, 682);
            ImGui::PushID(i);
            if (ImGui::InvisibleButton("nav", point(138, 51), ImGuiButtonFlags_EnableNav))
                tab = i;
            bool active = tab == i;
            ImU32 c = active ? accent() : muted;
            if (active) {
                d->AddRectFilled(point(x, 683), point(x + 138, 731), accent(14), 6 * scale);
                d->AddRectFilled(point(x + 15, 738), point(x + 123, 740), accent(), 2 * scale);
            }
            if (ImGui::IsItemHovered())
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            icon(d, i, point(x + 14, 697), c, .9f);
            drawText(d, x + 43, 698, names[i], c, app.graphics.medium, 13);
            ImGui::PopID();
        }
        drawText(d, 820, 691, "USER MODE", quiet, app.graphics.small, 10);
        drawText(d, 802, 710, "v3.0  /  Windows x64", muted, app.graphics.small, 10.5f);
        if (GetTickCount64() < app.toastUntil) {
            auto *fg = ImGui::GetForegroundDrawList();
            float tw = std::min(
                870.f,
                app.graphics.body->CalcTextSizeA(13 * scale, FLT_MAX, 0, app.toast.c_str()).x / scale + 38);
            float tx = 490 - tw / 2;
            fg->AddRectFilled(point(tx, 619), point(tx + tw, 661), IM_COL32(34, 33, 41, 252), 8 * scale);
            fg->AddRect(point(tx, 619), point(tx + tw, 661), accent(110), 8 * scale);
            fg->PushClipRect(point(tx + 12, 619), point(tx + tw - 12, 661), true);
            drawText(fg, tx + 18, 630, app.toast.c_str(), textColor, app.graphics.body, 13);
            fg->PopClipRect();
        }
        ImGui::End();
    }
};
} // namespace v6x
