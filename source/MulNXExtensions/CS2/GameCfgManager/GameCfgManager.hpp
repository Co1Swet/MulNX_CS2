// Cfg文件数据流向：
// 从工具目录读取Cfg文件->移动到游戏目录->调用游戏接口加载Cfg文件
// 工具目录允许移动和删除操作
// 游戏目录允许移动和加载操作
// 注意：不要允许直接从游戏目录删除Cfg文件，以免误删重要文件

// 目前的函数和成员命名是临时的，后续可能会调整以符合整体命名规范

#pragma once

#include <MulNX/MulNX.hpp>

class GameCfgManager final :public MulNX::ModuleBase {
private:
    MulNX::IPCer* IPCer = nullptr;

    std::filesystem::path ToolPath{};
    std::filesystem::path GamePath{};

    std::vector<std::string>ToolCfgs{};
    std::vector<std::string>GameCfgs{};
public:
    bool Init()override;

    //Cfg文件操作接口

    //更新工具目录和游戏目录的Cfg文件列表，在操作后调用
    bool UpdateCfgList();

    //从工具目录移动到游戏目录
    bool MoveToGame(const std::string& CfgName);
    //加载游戏目录的Cfg文件
    bool LoadCfg(const std::string& CfgName);
    //从游戏目录移动到工具目录
    bool MoveToTool(const std::string& CfgName);
    //从工具目录删除Cfg文件
    bool DeleteCfg(const std::string& CfgName);

    bool UINodeFunc(MulNX::UINode* node);
};