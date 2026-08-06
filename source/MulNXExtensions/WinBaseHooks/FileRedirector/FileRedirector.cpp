#include "FileRedirector.hpp"
#include "FileListenMixin.hpp"

bool FileRedirector::Init() {
    std::filesystem::path toolPath = this->Path()->PathGetForShared("Tools");

    this->pathGameinfo_gi = L"\\\\?\\" + (toolPath / "gameinfo.gi").wstring();
    this->pathMulNXPOV = L"\\\\?\\" + (toolPath / "MulNXPOV.vpk").wstring();

    this->hkCreateFileW = MulNX::Hook::Create((uint8_t*)&CreateFileW, [this](MulNX::Hook* hook, RegContext* ctx) {
        LPCWSTR lpFileName = reinterpret_cast<LPCWSTR>(ctx->rcx);
        if (!lpFileName)return MulNX::Hook::Then::Continue;
        std::wstring_view src(lpFileName);

        size_t prefixEnd = 0;
        if (src.starts_with(L"\\\\?\\"))
            prefixEnd = 4;
        else if (src.starts_with(L"\\??\\"))
            prefixEnd = 4;

        for(auto* listener : this->listeners) {
            auto res = listener->OnCreateFileW(hook, ctx);
            if (res.has_value())return res.value();
        }

        std::wstring_view cleanSrc = src.substr(prefixEnd);

        const std::wstring_view kKey = L"Counter-Strike Global Offensive";
        size_t pos = cleanSrc.find(kKey);
        if (pos == std::wstring_view::npos)return MulNX::Hook::Then::Continue;
        std::wstring_view suffix = cleanSrc.substr(pos);

        const std::wstring_view kExpectedSuffix =
            L"Counter-Strike Global Offensive\\game\\csgo\\gameinfo.gi";
        if (suffix.size() == kExpectedSuffix.size() &&
            _wcsnicmp(suffix.data(), kExpectedSuffix.data(), kExpectedSuffix.size()) == 0) {
            ctx->rcx = reinterpret_cast<uint64_t>(this->pathGameinfo_gi.c_str());
        }

        const std::wstring_view kExpectedSuffix2 =
            L"Counter-Strike Global Offensive\\game\\csgo\\MulNXPOV.vpk";
        if (suffix.size() == kExpectedSuffix2.size() &&
            _wcsnicmp(suffix.data(), kExpectedSuffix2.data(), kExpectedSuffix2.size()) == 0) {
            ctx->rcx = reinterpret_cast<uint64_t>(this->pathMulNXPOV.c_str());
        }

        return MulNX::Hook::Then::Continue;
        }).value();
    this->RegisterAttachHook(this->hkCreateFileW, "CreateFileW");

    return true;
}