#pragma once
#include <optional>
#include <string>
#include <string_view>

namespace cs2 {
inline std::optional<std::string> safeConfigName(std::string_view name) {
    while (!name.empty() && name.front() == ' ')
        name.remove_prefix(1);
    while (!name.empty() && name.back() == ' ')
        name.remove_suffix(1);
    if (name.empty() || name.size() > 40)
        return {};
    for (char c : name)
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ' ||
              c == '-' || c == '_'))
            return {};
    std::string upper(name);
    for (auto &c : upper)
        if (c >= 'a' && c <= 'z')
            c = static_cast<char>(c - 'a' + 'A');
    if (upper == "CON" || upper == "PRN" || upper == "AUX" || upper == "NUL")
        return {};
    if (upper.size() == 4 && (upper.substr(0, 3) == "COM" || upper.substr(0, 3) == "LPT") &&
        upper[3] >= '0' && upper[3] <= '9')
        return {};
    return std::string(name);
}
} // namespace cs2
