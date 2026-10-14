#include "../src/offset_data.hpp"
#include "../src/ini.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <fstream>
#include <iostream>
#include <functional>
using namespace v6x;
bool fails(const std::function<void()> &fn) {
    try {
        fn();
        return false;
    } catch (const std::exception &) {
        return true;
    }
}
int main() {
    std::ifstream g("reference/offsets.json"), s("reference/client_dll.json"), m("reference/provenance.json");
    Json globals, schema, meta;
    g >> globals;
    s >> schema;
    m >> meta;
    SnapshotMeta metadata{meta.at("source"),       meta.at("provider"),
                          meta.at("commit"),       meta.at("source_commit_date"),
                          meta.at("retrieved_on"), meta.at("build")};
    std::string text = makeOffsetIni(globals, schema, metadata);
    auto ini = Ini::parse(text);
    assert(ini.integer("globals", "dwEntityList", 0x40000000) ==
           checkedNumber(globals["client.dll"]["dwEntityList"], 0x40000000));
    assert(ini.get("metadata", "commit") == metadata.commit);
    assert(Ini::parse("\xef\xbb\xbf" + text).get("metadata", "commit") == metadata.commit);
    std::u16string wideText =
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.from_bytes(text);
    std::string utf16 = "\xff\xfe";
    for (char16_t c : wideText) {
        utf16.push_back(static_cast<char>(c & 255));
        utf16.push_back(static_cast<char>(c >> 8));
    }
    assert(Ini::parse(utf16).get("metadata", "commit") == metadata.commit);
    Json scalar = schema;
    for (const auto &f : fields)
        scalar["client.dll"]["classes"][f.type]["fields"][f.name] =
            checkedNumber(schema["client.dll"]["classes"][f.type]["fields"][f.name], 0x100000);
    assert(makeOffsetIni(globals, scalar, metadata) == text);
    for (const Json &bad : std::vector<Json>{true, false, 0, -1, 1.5, "123", nullptr, 0x50000000})
        assert(fails([&] { checkedNumber(bad, 0x40000000); }));
    assert(fails([&] { checkedNumber(Json::object(), 1000); }));
    assert(fails([&] { Ini::parse("[x]\na=1\na=2\n"); }));
    assert(fails([&] { Ini::parse("[x]\na=-1\n").integer("x", "a", 1000); }));
    assert(fails([&] { Ini::parse("[x]\na=0x10junk\n").integer("x", "a", 1000); }));
    auto invalid = metadata;
    invalid.commit = "../main";
    assert(fails([&] { makeOffsetIni(globals, schema, invalid); }));
    invalid = metadata;
    invalid.provider = "x\n[globals]";
    assert(fails([&] { makeOffsetIni(globals, schema, invalid); }));
    auto missing = globals;
    missing["client.dll"].erase("dwEntityList");
    assert(fails([&] { makeOffsetIni(missing, schema, metadata); }));
    std::cout << "PASS: scalar/typed schemas, required fields, bounds, UTF-8 BOM, UTF-16, duplicates and "
                 "metadata validation\n";
}
