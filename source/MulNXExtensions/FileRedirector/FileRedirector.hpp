#pragma once
#include <MulNX/MulNX.hpp>
#include <MulNXExtensions/WinExt/WinExt.hpp>

class FileRedirector final :public MulNX::Module<FileRedirector> {
    std::unique_ptr<MulNX::Hook> hkCreateFileW = nullptr;

    std::wstring pathGameinfo_gi;
    std::wstring pathMulNXPOV;
public:
    bool Init()override;
};