#pragma once
#include <Intro/CSModuleBase.hpp>
#include <MulNXExtensions/WinBaseHooks/FileRedirector/FileListenMixin.hpp>

class VPKInjector final :public CSModuleBase, public FileListenMixin<VPKInjector> {
    bool Init()override;
    std::optional<MulNX::Hook::Then> OnCreateFileW(MulNX::Hook* hk, RegContext* ctx)override;
};