#pragma once
#include <Intro/CSModuleBase.hpp>
#include <MulNXExtensions/WinBaseHooks/FileRedirector/FileListenMixin.hpp>
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

class RecordFileRedirect final :public CSModuleBase,
    public FileListenMixin<RecordFileRedirect>, public MediaModuleMixin<RecordFileRedirect> {
    
    std::filesystem::path dirVideos;
    std::atomic<std::shared_ptr<std::filesystem::path>> redirectBaseSnapshot{};
    bool Init()override;

    std::optional<MulNX::Hook::Then> OnCreateFileW(CreateFileWControl* pfc)override;
    std::optional<MulNX::Hook::Then> OnGetFileAttributesExW(GetFileAttributesExWControl* pac)override;
    std::optional<MulNX::Hook::Then> OnCreateDirectoryW(CreateDirectoryWControl* pdc)override;
};