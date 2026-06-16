#pragma once
#include <MulNX/Core/Module/Module.hpp>
#include <Windows.h>

namespace MulNX {
    class IPCer final :public MulNX::Module<IPCer> {
    public:
        bool Init()override;

        bool GetWindowPathByName(const LPCWSTR& WindowName, std::filesystem::path& Output);

        std::vector<std::string> GetProjectsNames(std::filesystem::path Path);
        std::vector<std::string> GetFileNamesByPath(std::filesystem::path& FolderPath);

        bool GetFileNames(std::vector<std::string>& FileNames, const std::filesystem::path& FolderPath, const std::vector<std::string>& Filter, const bool Extension = false);
        bool FileDelete(const std::string& FileName, const std::filesystem::path& FolderPath);
        bool FileMove(const std::string& FileName,
            const std::filesystem::path& Resource, const std::filesystem::path& Target);
    };
}