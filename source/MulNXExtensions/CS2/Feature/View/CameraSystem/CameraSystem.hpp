#pragma once
#include <Intro/CSModuleBase.hpp>
#include <Intro/HookView/CSViewControlModuleBase.hpp>
#include <MulNXExtensions/TimeLiner/TimeMixin.hpp>
#include "CameraDrawer/CameraDrawer.hpp"

#include <CameraSystem/ProjectManager/ProjectConfig.hpp>

class CampathManager;
class CamMacroManager;
class ProjectManager;

class Config {
public:
    ProjectConfig ProjectCfg{};

    std::pair<bool, std::string> Save(const std::filesystem::path& FolderPath);
};

class CameraSystem final :public CSModuleBase, public CSViewControlMixin<CameraSystem> {
    CampathManager* pCampathManager = nullptr;
    CamMacroManager* pCamMacroManager = nullptr;
    ProjectManager* PManager = nullptr;
    class CamPlayScheduler* pCamPlayScheduler = nullptr;
    MulNX::IPCer* pIPCer = nullptr;

    Config config{};

    void ProcessMsg(MulNX::Message& msg)override;
    void Window(MulNX::UICoordinator* uico);
    bool Init()override;
    bool HandleUpdateCSView(CS2::CViewSetup* viewSetup, const int& num, bool& camLeavePlayer)override;

    bool ConfigGenerate();
    bool ConfigSave();
    bool ConfigLoad();
    bool ConfigApply();
public:
    CameraDrawer CamDrawer{};
};

template <typename T>
class CamSysModuleMixin :public TimeMixin<T> {
    T* This() { return static_cast<T*>(this); }
public:
    CameraSystem* CamSys = nullptr;
protected:
    CamSysModuleMixin() {
        This()->preInits.push_back([this]() -> bool {
            this->CamSys = This()->FindModule<CameraSystem>("CameraSystem");
            return true;
            });
    }
};

class CamSysModule :public CSModuleBase, public CamSysModuleMixin<CamSysModule> {};