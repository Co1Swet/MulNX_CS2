#pragma once
#include <Intro/CSModuleBase.hpp>
#include <MulNXExtensions/WinBaseHooks/FileRedirector/FileListenMixin.hpp>

class RecordFileRedirect final :public CSModuleBase, public FileListenMixin<RecordFileRedirect> {
    std::filesystem::path dirVideos;

    bool Init()override;

    std::optional<MulNX::Hook::Then> OnCreateFileW(CreateFileWControl* pfc)override;
    std::optional<MulNX::Hook::Then> OnGetFileAttributesExW(GetFileAttributesExWControl* pac)override;
    std::optional<MulNX::Hook::Then> OnCreateDirectoryW(CreateDirectoryWControl* pdc)override;
};