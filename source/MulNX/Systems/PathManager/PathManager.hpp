#pragma once

// MulNX 框架的核心模块：路径管理器
// 由于同一项目中可能并存多个使用 MulNX 框架的二进制文件（如 CS2Injector.exe、CS2OBTool.dll），
// 为避免模块路径冲突，PathManager 要求每个二进制文件指定一个唯一的“核心名”（CoreName）。
// 核心名通常与二进制文件名一致，用于在路径中创建隔离的命名空间。
// 示例：
//   - 核心名 "CS2Injector" 下，模块 "CameraSystem" 的 Saves 目录：
//     MulNX/CS2Injector/CameraSystem/Saves
//   - 核心名 "CS2OBTool" 下，同一模块的 Saves 目录：
//     MulNX/CS2OBTool/CameraSystem/Saves
// 共享目录（如 Shared/Saves）不受核心名影响，为所有二进制文件共用。

#include <MulNX/Core/Module/Module.hpp>
#include "FilePathNode/FilePathNode.hpp"

namespace MulNX {
    class PathManager :public Module<PathManager> {
    private:
        std::recursive_mutex MutexEx;
        
        // MulNX目录（根目录）
        std::filesystem::path Root;
        // 核心名
        std::string CoreName;
        // 核心根目录
        std::filesystem::path CoreRoot;
        // 路径缓存
        std::unordered_map<std::string, std::filesystem::path>Cache;

        std::vector<std::string>Shareds;
        // 以字符串为Key
        std::unordered_map<std::string, FilePathNode> Nodes;

        FilePathNode* NodeGetFromKey(const std::string& Key);
        bool NodeHasRoot(FilePathNode* Node);
        bool KeyBindParentKey(const std::string& Key, const std::string& Parent);
        bool KeyUnbindParentKey(const std::string& Key);
        bool CallNodeChange(FilePathNode* Node);
    public:
        bool Init()override;

        bool LoadPathLists(const std::filesystem::path& xmlPath);
        bool CheckShared();

        const std::filesystem::path& GetRoot()const { return this->Root; }
        // 通过模块名，将目标（如Saves）映射到该模块的对应的目录（如ModuleA/Saves）
        std::filesystem::path PathGetForModule(const std::string& ModuleName, const std::string& Target);
        // 将目标映射到共享的目录
        std::filesystem::path PathGetForShared(const std::string& Target);

        bool CreateKey(const std::string& Key, std::string&& Value, std::function<bool(PathManager*)>&& OnChange);
        bool KeyBindStatic(const std::string& Key, const std::filesystem::path& Position);
        bool KeyBindDynamic(const std::string& Key, const std::string& Parent);
        bool KeySetCurrent(const std::string& Key, const std::string& Current);
        std::filesystem::path PathGetFromKey(const std::string& Key);
    };
}