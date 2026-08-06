#pragma once
#include <MulNX/MulNX.hpp>
#include <MulNXUtils/WinExt/HookMixin.hpp>

class IFileListenModule;

class CreateFileWControl;
class GetFileAttributesExWControl;

class FileRedirector final :public MulNX::Module<FileRedirector>, public HookMixin<FileRedirector> {
    std::unique_ptr<MulNX::Hook> hkCreateFileW = nullptr;
    std::unique_ptr<MulNX::Hook> hkGetFileAttributesExW = nullptr;

    bool Init()override;

    MulNX::Hook::Then OnCreateFileW(CreateFileWControl* pfc, RegContext* ctx);
    MulNX::Hook::Then OnGetFileAttributesExW(GetFileAttributesExWControl* pac, RegContext* ctx);
public:
    std::vector<IFileListenModule*> listeners;
};