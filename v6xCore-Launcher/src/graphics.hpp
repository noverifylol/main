#pragma once
#include "../assets/logo_frames.hpp"
#include "memory.hpp"
#include <d3d11.h>
#include <dxgi.h>
#include <gdiplus.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <objidl.h>
#include <vector>
#include <array>
#include <algorithm>

namespace v6x {
template <class T> void release(T *&p) {
    if (p) {
        p->Release();
        p = nullptr;
    }
}
struct Resource {
    void *data{};
    DWORD size{};
};
inline Resource resource(int id) {
    HRSRC r = FindResourceW(nullptr, MAKEINTRESOURCEW(id), RT_RCDATA);
    if (!r)
        return {};
    HGLOBAL h = LoadResource(nullptr, r);
    return {h ? LockResource(h) : nullptr, SizeofResource(nullptr, r)};
}
struct Graphics {
    ID3D11Device *device{};
    ID3D11DeviceContext *context{};
    IDXGISwapChain *swap{};
    ID3D11RenderTargetView *target{};
    ID3D11ShaderResourceView *logo{};
    UINT logoWidth{}, logoHeight{};
    ULONG_PTR gdiplus{};
    ImFont *body{}, *medium{}, *heading{}, *small{}, *display{}, *title{};
    struct Picture {
        ID3D11ShaderResourceView *view{};
        UINT width{}, height{};
    };
    std::array<Picture, 8> covers{};
    bool initialized{};
    void createTarget() {
        ID3D11Texture2D *b{};
        if (swap && SUCCEEDED(swap->GetBuffer(0, IID_PPV_ARGS(&b)))) {
            device->CreateRenderTargetView(b, nullptr, &target);
            b->Release();
        }
    }
    bool initialize(HWND window) {
        DXGI_SWAP_CHAIN_DESC sd{};
        sd.BufferCount = 2;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = window;
        sd.SampleDesc.Count = 1;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        const D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
        D3D_FEATURE_LEVEL obtained{};
        HRESULT result =
            D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, levels, 2,
                                          D3D11_SDK_VERSION, &sd, &swap, &device, &obtained, &context);
        if (FAILED(result))
            result =
                D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, levels, 2,
                                              D3D11_SDK_VERSION, &sd, &swap, &device, &obtained, &context);
        if (FAILED(result))
            return false;
        createTarget();
        if (!target)
            return false;
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        auto &io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.LogFilename = nullptr;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        auto font = [&](int id, float size) {
            const auto r = resource(id);
            if (!r.data || !r.size)
                return io.Fonts->AddFontDefault();
            ImFontConfig cfg{};
            cfg.FontDataOwnedByAtlas = false;
            cfg.OversampleH = 2;
            cfg.OversampleV = 2;
            return io.Fonts->AddFontFromMemoryTTF(r.data, static_cast<int>(r.size), size, &cfg,
                                                  io.Fonts->GetGlyphRangesDefault());
        };
        body = font(201, 14);
        medium = font(202, 14);
        heading = font(202, 24);
        small = font(201, 11.5f);
        display = font(202, 44);
        title = font(202, 32);
        io.FontDefault = body;
        if (!ImGui_ImplWin32_Init(window) || !ImGui_ImplDX11_Init(device, context))
            return false;
        initialized = true;
        Gdiplus::GdiplusStartupInput gi{};
        if (Gdiplus::GdiplusStartup(&gdiplus, &gi, nullptr) == Gdiplus::Ok) {
            loadTexture(102, logo, logoWidth, logoHeight);
            for (size_t i = 0; i < covers.size(); ++i)
                loadTexture(501 + static_cast<int>(i), covers[i].view, covers[i].width, covers[i].height);
        }
        return true;
    }
    void loadTexture(int id, ID3D11ShaderResourceView *&view, UINT &imageWidth, UINT &imageHeight) {
        const auto r = resource(id);
        if (!r.data || !r.size)
            return;
        HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, r.size);
        if (!mem)
            return;
        void *data = GlobalLock(mem);
        if (!data) {
            GlobalFree(mem);
            return;
        }
        std::memcpy(data, r.data, r.size);
        GlobalUnlock(mem);
        IStream *stream{};
        if (FAILED(CreateStreamOnHGlobal(mem, TRUE, &stream))) {
            GlobalFree(mem);
            return;
        }
        {
            Gdiplus::Bitmap bitmap(stream, FALSE);
            if (bitmap.GetLastStatus() == Gdiplus::Ok) {
                imageWidth = bitmap.GetWidth();
                imageHeight = bitmap.GetHeight();
                Gdiplus::Rect rect(0, 0, static_cast<int>(imageWidth), static_cast<int>(imageHeight));
                Gdiplus::BitmapData pixels{};
                if (bitmap.LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &pixels) ==
                    Gdiplus::Ok) {
                    std::vector<unsigned char> bytes(static_cast<size_t>(imageWidth) * imageHeight * 4);
                    for (UINT y = 0; y < imageHeight; ++y)
                        std::memcpy(bytes.data() + static_cast<size_t>(y) * imageWidth * 4,
                                    static_cast<unsigned char *>(pixels.Scan0) +
                                        static_cast<ptrdiff_t>(y) * pixels.Stride,
                                    imageWidth * 4);
                    bitmap.UnlockBits(&pixels);
                    D3D11_TEXTURE2D_DESC td{};
                    td.Width = imageWidth;
                    td.Height = imageHeight;
                    td.MipLevels = 1;
                    td.ArraySize = 1;
                    td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
                    td.SampleDesc.Count = 1;
                    td.Usage = D3D11_USAGE_IMMUTABLE;
                    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
                    D3D11_SUBRESOURCE_DATA sd{};
                    sd.pSysMem = bytes.data();
                    sd.SysMemPitch = imageWidth * 4;
                    ID3D11Texture2D *tex{};
                    if (SUCCEEDED(device->CreateTexture2D(&td, &sd, &tex))) {
                        device->CreateShaderResourceView(tex, nullptr, &view);
                        tex->Release();
                    }
                }
            }
        }
        stream->Release();
    }
    void drawLogo(ImDrawList *draw, ImVec2 start, float size, bool animate, int alpha = 255) {
        if (!logo)
            return;
        unsigned time = animate ? static_cast<unsigned>(ImGui::GetTime() * 1000.0) % v6xLogo::duration : 0;
        int index = 0;
        while (index < v6xLogo::frames - 1 && time >= v6xLogo::durations[index]) {
            time -= v6xLogo::durations[index];
            ++index;
        }
        const float u = static_cast<float>((index % v6xLogo::columns) * v6xLogo::width) / logoWidth;
        const float v = static_cast<float>((index / v6xLogo::columns) * v6xLogo::height) / logoHeight;
        draw->AddImage(static_cast<ImTextureID>(reinterpret_cast<intptr_t>(logo)), start,
                       {start.x + size, start.y + size}, {u, v},
                       {u + static_cast<float>(v6xLogo::width) / logoWidth,
                        v + static_cast<float>(v6xLogo::height) / logoHeight},
                       IM_COL32(255, 255, 255, alpha));
    }
    void drawCover(ImDrawList *draw, size_t index, ImVec2 a, ImVec2 b, int alpha = 255, float rounding = 0) {
        if (index >= covers.size() || !covers[index].view)
            return;
        const auto &t = covers[index];
        float ratio = (b.x - a.x) / std::max(1.f, b.y - a.y), actual = static_cast<float>(t.width) / t.height;
        ImVec2 uv0{0, 0}, uv1{1, 1};
        if (actual > ratio) {
            float amount = ratio / actual;
            uv0.x = (1 - amount) * .72f;
            uv1.x = uv0.x + amount;
        } else {
            float amount = actual / ratio;
            uv0.y = (1 - amount) * .17f;
            uv1.y = uv0.y + amount;
        }
        draw->AddImageRounded(static_cast<ImTextureID>(reinterpret_cast<intptr_t>(t.view)), a, b, uv0, uv1,
                              IM_COL32(255, 255, 255, alpha), rounding);
    }
    void resize(UINT w, UINT h) {
        if (!device || !w || !h)
            return;
        release(target);
        context->OMSetRenderTargets(0, nullptr, nullptr);
        if (SUCCEEDED(swap->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 0)))
            createTarget();
    }
    void begin() {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }
    void present() {
        ImGui::Render();
        if (!target)
            return;
        constexpr float clear[] = {0.055f, 0.055f, 0.067f, 1};
        context->OMSetRenderTargets(1, &target, nullptr);
        context->ClearRenderTargetView(target, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        swap->Present(0, DXGI_PRESENT_DO_NOT_WAIT);
    }
    ~Graphics() {
        if (initialized) {
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
        }
        release(logo);
        for (auto &cover : covers)
            release(cover.view);
        release(target);
        release(swap);
        release(context);
        release(device);
        if (gdiplus)
            Gdiplus::GdiplusShutdown(gdiplus);
    }
};
} // namespace v6x
