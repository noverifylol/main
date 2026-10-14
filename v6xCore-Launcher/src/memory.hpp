#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <tlhelp32.h>
#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>
#include <utility>

namespace cs2 {
using Address = std::uintptr_t;
static_assert(sizeof(Address) == 8, "Build for Windows x64.");

class Handle {
    HANDLE value_{};

  public:
    Handle() = default;
    explicit Handle(HANDLE h) : value_(h) {}
    ~Handle() {
        reset();
    }
    Handle(const Handle &) = delete;
    Handle &operator=(const Handle &) = delete;
    Handle(Handle &&h) noexcept : value_(std::exchange(h.value_, nullptr)) {}
    Handle &operator=(Handle &&h) noexcept {
        if (this != &h) {
            reset();
            value_ = std::exchange(h.value_, nullptr);
        }
        return *this;
    }
    HANDLE get() const {
        return value_;
    }
    explicit operator bool() const {
        return value_ && value_ != INVALID_HANDLE_VALUE;
    }
    void reset(HANDLE next = nullptr) {
        if (*this)
            CloseHandle(value_);
        value_ = next;
    }
};

inline bool userPointer(Address p) {
    return p >= 0x10000 && p < 0x0000800000000000ull;
}

class Process {
    Handle handle_;
    DWORD pid_{}, foundPid_{}, lastError_{};
    Address client_{};
    std::size_t clientSize_{};

  public:
    std::wstring status = L"cs2.exe not found; waiting for game startup.";
    void close() {
        handle_.reset();
        pid_ = 0;
        foundPid_ = 0;
        lastError_ = 0;
        client_ = 0;
        clientSize_ = 0;
    }
    bool alive() const {
        DWORD code{};
        return handle_ && GetExitCodeProcess(handle_.get(), &code) && code == STILL_ACTIVE;
    }
    DWORD foundPid() const {
        return foundPid_;
    }
    DWORD lastError() const {
        return lastError_;
    }
    DWORD pid() const {
        return pid_;
    }
    Address client() const {
        return client_;
    }
    std::size_t clientSize() const {
        return clientSize_;
    }
    bool attach() {
        if (alive() && client_)
            return true;
        close();
        Handle processes(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
        PROCESSENTRY32W pe{};
        pe.dwSize = sizeof(pe);
        if (!processes || !Process32FirstW(processes.get(), &pe)) {
            lastError_ = GetLastError();
            status = L"Could not read process list (error " + std::to_wstring(lastError_) + L").";
            return false;
        }
        do {
            if (_wcsicmp(pe.szExeFile, L"cs2.exe") == 0) {
                pid_ = pe.th32ProcessID;
                foundPid_ = pid_;
                break;
            }
        } while (Process32NextW(processes.get(), &pe));
        if (!pid_) {
            status = L"cs2.exe not found; waiting for game startup.";
            return false;
        }
        // Deliberately no VM_WRITE, VM_OPERATION, debug privilege, driver or injection.
        handle_.reset(OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid_));
        if (!handle_) {
            lastError_ = GetLastError();
            status = L"cs2.exe found, read access denied (error " + std::to_wstring(lastError_) +
                     L"). No access bypass.";
            pid_ = 0;
            return false;
        }
        Handle modules(CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid_));
        MODULEENTRY32W me{};
        me.dwSize = sizeof(me);
        if (modules && Module32FirstW(modules.get(), &me)) {
            do {
                if (_wcsicmp(me.szModule, L"client.dll") == 0) {
                    client_ = reinterpret_cast<Address>(me.modBaseAddr);
                    clientSize_ = me.modBaseSize;
                    break;
                }
            } while (Module32NextW(modules.get(), &me));
        }
        if (!client_) {
            lastError_ = GetLastError();
            status = L"cs2.exe found, client.dll not readable yet. Error " + std::to_wstring(lastError_);
            handle_.reset();
            pid_ = 0;
            client_ = 0;
            clientSize_ = 0;
            return false;
        }
        status = L"client.dll attached";
        return true;
    }
    bool bytes(Address address, void *output, std::size_t size) const {
        if (!handle_ || !userPointer(address) || size == 0 || size > 1024 * 1024 ||
            address > 0x0000800000000000ull - size)
            return false;
        SIZE_T got{};
        return ReadProcessMemory(handle_.get(), reinterpret_cast<const void *>(address), output, size,
                                 &got) &&
               got == size;
    }
    template <class T> bool read(Address address, T &out) const {
        static_assert(std::is_trivially_copyable_v<T>);
        T value{};
        if (!bytes(address, &value, sizeof(value)))
            return false;
        out = value;
        return true;
    }
    Address pointer(Address address) const {
        Address p{};
        return read(address, p) && userPointer(p) ? p : 0;
    }
};

inline HWND gameWindow(DWORD pid) {
    struct Search {
        DWORD pid;
        HWND found{};
        long long area{};
    } search{pid};
    EnumWindows(
        [](HWND window, LPARAM param) -> BOOL {
            auto &s = *reinterpret_cast<Search *>(param);
            DWORD id{};
            GetWindowThreadProcessId(window, &id);
            if (id != s.pid || !IsWindowVisible(window) || GetWindow(window, GW_OWNER))
                return TRUE;
            RECT r{};
            if (GetClientRect(window, &r)) {
                long long area = static_cast<long long>(r.right) * r.bottom;
                if (r.right >= 200 && r.bottom >= 200 && area > s.area) {
                    s.area = area;
                    s.found = window;
                }
            }
            return TRUE;
        },
        reinterpret_cast<LPARAM>(&search));
    return search.found;
}
inline bool foregroundIs(DWORD pid) {
    DWORD active{};
    GetWindowThreadProcessId(GetForegroundWindow(), &active);
    return pid != 0 && active == pid;
}
} // namespace cs2
