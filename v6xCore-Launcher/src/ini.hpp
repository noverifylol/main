#pragma once
#include <algorithm>
#include <charconv>
#include <codecvt>
#include <cstdint>
#include <locale>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace v6x {
inline std::string trim(std::string value) {
    auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return {};
    auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}
inline std::string lowerAscii(std::string text) {
    for (auto &c : text)
        if (c >= 'A' && c <= 'Z')
            c = static_cast<char>(c - 'A' + 'a');
    return text;
}
inline std::string decodeIni(const std::string &bytes) {
    if (bytes.size() >= 3 && bytes.compare(0, 3, "\xef\xbb\xbf") == 0)
        return bytes.substr(3);
    if (bytes.size() >= 2 &&
        ((static_cast<unsigned char>(bytes[0]) == 0xff && static_cast<unsigned char>(bytes[1]) == 0xfe) ||
         (static_cast<unsigned char>(bytes[0]) == 0xfe && static_cast<unsigned char>(bytes[1]) == 0xff))) {
        if ((bytes.size() - 2) % 2)
            throw std::runtime_error("Invalid UTF-16 INI length");
        bool little = static_cast<unsigned char>(bytes[0]) == 0xff;
        std::u16string utf16;
        for (size_t i = 2; i < bytes.size(); i += 2) {
            auto a = static_cast<unsigned char>(bytes[i]), b = static_cast<unsigned char>(bytes[i + 1]);
            utf16.push_back(static_cast<char16_t>(little ? a | (b << 8) : (a << 8) | b));
        }
        return std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.to_bytes(utf16);
    }
    if (bytes.find('\0') != std::string::npos)
        throw std::runtime_error("INI contains binary or unsupported encoding");
    return bytes;
}
struct Ini {
    std::map<std::string, std::map<std::string, std::string>> sections;
    static Ini parse(const std::string &bytes) {
        Ini result;
        std::istringstream in(decodeIni(bytes));
        std::string line, section;
        while (std::getline(in, line)) {
            line = trim(line);
            if (line.empty() || line[0] == ';' || line[0] == '#')
                continue;
            if (line.front() == '[' && line.back() == ']') {
                section = lowerAscii(trim(line.substr(1, line.size() - 2)));
                continue;
            }
            auto equals = line.find('=');
            if (equals == std::string::npos || section.empty())
                throw std::runtime_error("Malformed INI entry");
            std::string key = lowerAscii(trim(line.substr(0, equals))), value = trim(line.substr(equals + 1));
            if (key.empty() || result.sections[section].count(key))
                throw std::runtime_error("Empty or duplicate INI key: " + key);
            if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
                value = value.substr(1, value.size() - 2);
            result.sections[section][key] = value;
        }
        return result;
    }
    std::string get(const std::string &section, const std::string &key,
                    const std::string &fallback = "") const {
        auto s = sections.find(lowerAscii(section));
        if (s == sections.end())
            return fallback;
        auto v = s->second.find(lowerAscii(key));
        return v == s->second.end() ? fallback : v->second;
    }
    std::uint64_t integer(const std::string &section, const std::string &key, std::uint64_t maximum) const {
        std::string text = get(section, key);
        if (text.empty())
            throw std::runtime_error("Missing offset: " + section + "/" + key);
        int base = 10;
        std::string_view view = text;
        if (view.size() > 2 && view[0] == '0' && (view[1] == 'x' || view[1] == 'X')) {
            base = 16;
            view.remove_prefix(2);
        }
        std::uint64_t value{};
        auto result = std::from_chars(view.data(), view.data() + view.size(), value, base);
        if (result.ec != std::errc{} || result.ptr != view.data() + view.size() || !value || value > maximum)
            throw std::runtime_error("Invalid offset: " + section + "/" + key + " = " + text);
        return value;
    }
};
} // namespace v6x
