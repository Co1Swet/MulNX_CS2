#pragma once
#include "ProjectConfig.hpp"
#include <CameraSystem/CameraSystem.hpp>
#include <CameraSystem/Project/Project.hpp>

//项目管理器，用于管理项目
class ProjectManager final :public CamSysModule {
private:
    ElementManager* EManager = nullptr;
    SolutionManager* SManager = nullptr;
    MulNX::IPCer* pIPCer = nullptr;

    std::atomic<bool> OpenProjectKCPackDebugWindow = false;
    std::unordered_map<std::string, std::shared_ptr<Project>> projects{};

    //快捷键修改缓存
    MulNX::KeyCheckPack Buffer_KCPack{};
public:
    //当前操作项目指针（操作对象）
    std::shared_ptr<Project> ControllingProject = nullptr;
    //当前活跃对象
    std::shared_ptr<Project> ActiveProject = nullptr;

    ProjectConfig Config{};

    void Project_DebugWindow();
    bool MenuProject();
    bool UINodeFunc();

    bool Init()override;
    void HandleUpdate();

    //卸载项目（从内存中移除）
    bool Project_Delete(const std::string& name);
    //清空项目（从内存中移除）
    bool Project_ClearAll();

    //创建项目
    bool Project_Create(const std::string& name);
    //保存活跃项目
    bool Project_Save();
    //刷新活跃项目
    bool Project_Refresh();
    //保存所有项目
    //bool Project_SaveAll();
    //切换项目，重载解决方案和元素（删除先删解决方案再删元素，加载先加载元素再加载解决方案）
    bool Project_Apply(const std::shared_ptr<Project> project);
    //从文件加载项目
    bool Project_Load(const std::filesystem::path& projectPath, const std::string& name);

    //通过信息设置当前播放的解决方案（0：默认游戏时间轴播放，1：偏移时间轴播放）
    bool Playing_AutoCall(const MulNX::Message& msg);
};