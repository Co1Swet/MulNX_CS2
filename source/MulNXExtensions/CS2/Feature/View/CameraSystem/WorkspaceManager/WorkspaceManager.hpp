#pragma once
#include <CameraSystem/Workspace/Workspace.hpp>
#include <CameraSystem/CameraSystem.hpp>

class WorkspaceManager final :public CamSysModule {
private:
    ElementManager* EManager = nullptr;
    SolutionManager* SManager = nullptr;
    ProjectManager* PManager = nullptr;
    MulNX::IPCer* pIPCer = nullptr;

    std::unique_ptr<Workspace> CurrentWorkspace = nullptr;
    bool MenuWorkspace();
    void ProcessMsg(MulNX::Message& msg)override;
public:
    std::atomic<bool> InWorkspace = false;
    void HandleUpdate();

    bool Init()override;

    //工作区

    //保存工作区所有内容到磁盘中
    bool Workspace_Save();
    //真正切换到目标工作区并清空当前数据（在开始会先调用Workspace_TrySetPath进行检测，通过后才会从当前工作区退出）
    bool Workspace_Set(const std::string& Name);


    //工作区配置相关

    //生成配置到内存中
    bool Workspace_ConfigGenerate();
    //将内存中的配置保存到磁盘中
    bool Workspace_ConfigSave();
    //加载配置到内存中
    bool Workspace_ConfigLoad(const std::filesystem::path& WorkspacePath);
    //应用内存中的配置产生实际效果
    bool Workspace_ConfigApply();
};