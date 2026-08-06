#include "VPKInjector.hpp"

bool VPKInjector::Init() {
    std::filesystem::path toolPath = this->Path()->PathGetForShared("Tools");

    this->pathGameinfo_gi = L"\\\\?\\" + (toolPath / "gameinfo.gi").wstring();
    this->pathMulNXPOV = L"\\\\?\\" + (toolPath / "MulNXPOV.vpk").wstring();

    return true;
}

std::optional<MulNX::Hook::Then> VPKInjector::OnCreateFileW(FileListenControl* pfc) {
    auto cleanSrc = pfc->GetCleanSrc();

    const std::wstring_view kKey = L"Counter-Strike Global Offensive";
    size_t pos = cleanSrc.find(kKey);
    if (pos == std::wstring_view::npos)return MulNX::Hook::Then::Continue;
    std::wstring_view suffix = cleanSrc.substr(pos);

    const std::wstring_view kExpectedSuffix =
        L"Counter-Strike Global Offensive\\game\\csgo\\gameinfo.gi";
    if (suffix.size() == kExpectedSuffix.size() &&
        _wcsnicmp(suffix.data(), kExpectedSuffix.data(), kExpectedSuffix.size()) == 0) {
        pfc->redirected = &this->pathGameinfo_gi;
        return MulNX::Hook::Then::Continue;
    }

    const std::wstring_view kExpectedSuffix2 =
        L"Counter-Strike Global Offensive\\game\\csgo\\MulNXPOV.vpk";
    if (suffix.size() == kExpectedSuffix2.size() &&
        _wcsnicmp(suffix.data(), kExpectedSuffix2.data(), kExpectedSuffix2.size()) == 0) {
        pfc->redirected = &this->pathMulNXPOV;
        return MulNX::Hook::Then::Continue;
    }

    return std::nullopt;
}