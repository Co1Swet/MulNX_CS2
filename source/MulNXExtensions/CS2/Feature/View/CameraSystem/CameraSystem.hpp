#pragma once
#include <Intro/Intros.hpp>
#include <Intro/HookView/CSViewControlModuleBase.hpp>
#include <MulNXExtensions/TimeLiner/TimeMixin.hpp>

class CameraSystemIO {
public:
    // 单位为秒的时间，当采用CS2时间时，它对应高精度DemoTime（不是GameTime）
    double time;
    // 当前帧数据
    MulNX::Math::View view;
};

class CamDrawer;

class CameraSystem final :public CSModuleBase, public CSViewControlMixin<CameraSystem> {
    CamDrawer* pCamDrawer = nullptr;
    class CampathManager* pCampathManager = nullptr;
    class CamMacroManager* pCamMacroManager = nullptr;
    class CamPackManager* pCamPackManager = nullptr;
    class CamPlayScheduler* pCamPlayScheduler = nullptr;
    
    void ProcessMsg(MulNX::Message& msg)override;
    void Window(MulNX::UICoordinator* uico);
    bool Init()override;
    bool HandleUpdateCSView(CS2::CViewSetup* viewSetup, const int& num, bool& camLeavePlayer)override;    
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