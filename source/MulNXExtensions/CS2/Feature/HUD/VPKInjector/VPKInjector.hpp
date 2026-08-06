#pragma once
#include <Intro/CSModuleBase.hpp>
#include <MulNXExtensions/WinBaseHooks/FileRedirector/FileListenMixin.hpp>

class VPKInjector final :public CSModuleBase, public FileListenMixin<VPKInjector> {
    std::wstring pathGameinfo_gi;
    std::wstring pathMulNXPOV;

    bool Init()override;
    std::optional<MulNX::Hook::Then> OnCreateFileW(FileListenControl* pfc)override;
};