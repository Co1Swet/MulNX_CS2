#pragma once
#include "ProjectConfig.hpp"
#include <CameraSystem/CameraSystem.hpp>
#include <CameraSystem/Project/Project.hpp>

//项目管理器，用于管理项目
class ProjectManager final :public CamSysModule {
    MulNX::IPCer* pIPCer = nullptr;
    std::unordered_map<std::string, std::shared_ptr<Project>> projects{};

    std::atomic<bool> OpenProjectKCPackDebugWindow = false;
    mutable std::atomic<MulNX::KeyCheckPack> bufKCPack{};

    // 当前操作项目指针（操作对象）
    std::shared_ptr<Project> ControllingProject = nullptr;

    void Project_DebugWindow();
    void UINodeFunc();

    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;

    // 卸载项目（从内存中移除）
    bool Project_Delete(const std::string& name);
    // 清空项目（从内存中移除）
    bool Project_ClearAll();
    // 创建项目
    bool Project_Create(const std::string& name);
    // 保存活跃项目
    bool Project_Save();
    // 刷新活跃项目
    bool Project_Refresh();
    // 从文件加载项目
    bool Project_Load(const std::filesystem::path& projectPath, const std::string& name);
    // 切换项目，重载解决方案和元素（删除先删解决方案再删元素，加载先加载元素再加载解决方案）
    bool Project_Apply(const std::shared_ptr<Project> project);
public:
    //当前活跃对象
    std::shared_ptr<Project> ActiveProject = nullptr;
    ProjectConfig Config{};

    bool MenuProject();
    void HandleUpdate();
};