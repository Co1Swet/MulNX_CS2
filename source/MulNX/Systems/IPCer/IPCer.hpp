#pragma once
#include <MulNX/Core/Module/Module.hpp>
#include <Windows.h>

namespace MulNX {
    class IPCer final :public MulNX::Module<IPCer> {
        bool Init()override;
    public:
        bool GetWindowPathByName(const LPCWSTR& WindowName, fs::path& Output);

        std::vector<std::string> GetDirNamesByPath(fs::path Path);
        std::vector<std::string> GetFileNamesByPath(fs::path& FolderPath);

        bool GetFileNames(std::vector<std::string>& FileNames, const fs::path& FolderPath, const std::vector<std::string>& Filter, const bool Extension = false);
        bool FileDelete(const std::string& FileName, const fs::path& FolderPath);
        bool FileMove(const std::string& FileName,
            const fs::path& Resource, const fs::path& Target);
    };
}