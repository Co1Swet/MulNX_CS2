#pragma once
#include "SolutionConfig.hpp"
#include <CameraSystem/Solution/Solution.hpp>
#include <CameraSystem/CameraSystem.hpp>

//解决方案管理器，用于管理解决方案
class SolutionManager final :public CamSysModule {
    MulNX::IPCer* pIPCer = nullptr;
    //数据存储
    std::unordered_map<std::string, std::unique_ptr<Solution>> solutions{};
    //当前操作的解决方案指针
    Solution* CurrentSolution = nullptr;
    //按键调试缓存
    MulNX::KeyCheckPack Buffer_KCPack{};
    //是否打开解决方案按键绑定调试窗口
    std::atomic<bool> OpenSolutionKCPackDebugWindow = false;

    bool UINodeFunc();
    void Solution_ShowInLine(const Solution* solution)const;
    void Solution_DebugWindow(const Solution* pMacro)const;

    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;

    Solution* FindCamMacro(const std::string& name);

    bool Solution_Create(const std::string& Name);
    //保存所有解决方案到文件
    bool Solution_SaveAll();
    //从文件加载解决方案（反序列化，序列化在Solution）
    bool Solution_Load(const std::filesystem::path& FullPath);
    //删除解决方案
    bool Solution_Delete(const std::string& Name);
    //删除所有解决方案
    bool Solution_ClearAll();

    //预览功能相关

    //通过名称设置当前播放的解决方案
    void Playing_Solution(const std::string& SolutionName);
public:
    SolutionConfig Config{};
    bool MenuSolution();
    bool HandleUpdate(CameraSystemIO* IO);
};