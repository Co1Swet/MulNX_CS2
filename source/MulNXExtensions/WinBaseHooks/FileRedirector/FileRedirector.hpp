#pragma once
#include <MulNX/MulNX.hpp>
#include <MulNXUtils/WinExt/HookMixin.hpp>

class IFileListenModule;
class FileRedirector final :public MulNX::Module<FileRedirector>, public HookMixin<FileRedirector> {
    std::unique_ptr<MulNX::Hook> hkCreateFileW = nullptr;

    std::wstring pathGameinfo_gi;
    std::wstring pathMulNXPOV;
    bool Init()override;
public:
    std::vector<IFileListenModule*> listeners;
};