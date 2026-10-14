#pragma once
#include "config.hpp"
#include "config_name.hpp"
#include "file_io.hpp"
#include <vector>
namespace v6x {
struct BootstrapResult {
    std::vector<std::string> messages;
    bool offsetsCreated{};
};
inline BootstrapResult bootstrap(const std::filesystem::path &directory) {
    BootstrapResult result;
    // Copy legacy named profiles once, without overwriting or deleting user files.
    std::error_code migrationError;
    const auto legacy = directory / "profiles", configs = directory / "configs";
    if (std::filesystem::is_directory(legacy, migrationError)) {
        std::filesystem::create_directories(configs, migrationError);
        for (std::filesystem::directory_iterator it(legacy, migrationError), end;
             !migrationError && it != end; it.increment(migrationError)) {
            if (it->is_regular_file(migrationError) && !it->is_symlink(migrationError) &&
                it->path().extension() == ".ini") {
                auto name = cs2::safeConfigName(it->path().stem().string());
                if (name && !std::filesystem::exists(configs / (*name + ".ini"), migrationError)) {
                    std::filesystem::copy_file(it->path(), configs / (*name + ".ini"),
                                               std::filesystem::copy_options::none, migrationError);
                    if (!migrationError)
                        result.messages.push_back("Imported legacy config: " + *name);
                }
            }
        }
    }
    const std::pair<int, const wchar_t *> files[] = {
        {301, L"offsets.ini"},          {302, L"settings.ini"},
        {303, L"configs/Balanced.ini"}, {304, L"configs/Minimal.ini"},
        {305, L"configs/Ice.ini"},      {401, L"licenses/THIRD_PARTY_LICENSES.txt"}};
    for (const auto &item : files) {
        auto path = directory / item.second;
        std::error_code e;
        bool exists = std::filesystem::exists(path, e);
        if (e)
            throw std::runtime_error("Cannot inspect data file: " + utf8(path.wstring()));
        if (!exists) {
            std::string data = bundledText(item.first);
            if (item.first == 301) {
                cs2::Offsets test;
                if (!test.loadText(data))
                    throw std::runtime_error("Invalid embedded offsets: " + utf8(test.error));
            }
            atomicWrite(path, data);
            result.messages.push_back("Created bundled file: " + utf8(path.wstring()));
            if (item.first == 301)
                result.offsetsCreated = true;
        }
    }
    return result;
}
inline void restoreBundledOffsets(const std::filesystem::path &directory) {
    std::string data = bundledText(301);
    cs2::Offsets check;
    if (!check.loadText(data))
        throw std::runtime_error("Embedded offset data is invalid");
    atomicWrite(directory / "offsets.ini", data, true);
}
} // namespace v6x
