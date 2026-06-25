#pragma once
#include <Intro/CSModuleBase.hpp>

class GameCfgManager final :public CSModuleBase {
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