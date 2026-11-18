#pragma once
#include <MulNX/Core/Module/Module.hpp>
#include "FilePathNode/FilePathNode.hpp"

namespace MulNX {
    class PathManager :public Module<PathManager> {
        std::recursive_mutex recursive_mutex;
        // MulNX目录（根目录）
        fs::path Root;
        // 核心名
        std::string CoreName;
        // 核心根目录
        fs::path CoreRoot;
        // 路径缓存
        std::unordered_map<std::string, fs::path>Cache;

        std::vector<std::string>Shareds;
        // 以字符串为Key
        std::unordered_map<std::string, FilePathNode> Nodes;

        FilePathNode* NodeGetFromKey(const std::string& Key);
        bool NodeHasRoot(FilePathNode* Node);
        bool KeyBindParentKey(const std::string& Key, const std::string& Parent);
        bool KeyUnbindParentKey(const std::string& Key);
        bool CallNodeChange(FilePathNode* Node);

        bool Init()override;
    public:
        bool LoadPathLists(const fs::path& xmlPath);
        bool CheckShared();

        const fs::path& GetRoot()const { return this->Root; }
        // 通过模块名，将目标（如Saves）映射到该模块的对应的目录（如ModuleA/Saves）
        fs::path PathGetForModule(const std::string_view moduleName, const std::string& Target);
        // 将目标映射到共享的目录
        fs::path PathGetForShared(const std::string& Target);

        void CreateKey(const std::string& Key, std::string&& Value, std::function<bool(PathManager*)>&& OnChange);
        bool KeyBindStatic(const std::string& Key, const fs::path& Position);
        bool KeyBindDynamic(const std::string& Key, const std::string& Parent);
        bool KeySetCurrent(const std::string& Key, const std::string& Current);
        std::optional<fs::path> PathGetFromKey(const std::string& Key);

        std::vector<std::string> PathFindStaticKeys(const fs::path& Target);
    };
}