#pragma once

#include <MulNX/Core/ModuleBase/ModuleBase.hpp>

#include <Windows.h>

class Paths {
public:
    //std::filesystem::path GetPath();
    // 主路径结构
    struct S_MulNX {
        std::filesystem::path Path;

    } MulNX;

    struct S_Counter_Strike_Global_Offensive {
        std::filesystem::path Path;

        struct S_game {
            std::filesystem::path Path;

            struct S_bin {
                std::filesystem::path Path;

                struct S_win64 {
                    std::filesystem::path Path;

                    struct S_cs2_exe {
                        std::filesystem::path Path;
                    }cs2_exe;
                } win64;
			} bin;

            struct S_csgo {
                std::filesystem::path Path;

                struct S_cfg {
                    std::filesystem::path Path;
				} cfg;
			} csgo;
        }game;
	} Counter_Strike_Global_Offensive;
};

namespace MulNX {
    class IPCer final :public MulNX::ModuleBase {
        Paths Paths{};
    public:
        bool Init()override;

        bool GetWindowPathByName(const LPCWSTR& WindowName, std::filesystem::path& Output);

        std::filesystem::path PathGet_CS_cfg();

        std::vector<std::string> GetProjectsNames(std::filesystem::path Path);
        std::vector<std::string> GetFileNamesByPath(std::filesystem::path& FolderPath);

        bool GetFileNames(std::vector<std::string>& FileNames, const std::filesystem::path& FolderPath, const std::vector<std::string>& Filter, const bool Extension = false);
        bool FileDelete(const std::string& FileName, const std::filesystem::path& FolderPath);
        bool FileMove(const std::string& FileName,
            const std::filesystem::path& Resource, const std::filesystem::path& Target);

        std::filesystem::path GetRoot();
    };
}