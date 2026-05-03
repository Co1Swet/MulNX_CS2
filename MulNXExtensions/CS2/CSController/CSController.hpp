#pragma once

#include <MulNX/MulNX.hpp>
#include <MulNX/Base/Math/Math.hpp>
#include <MulNX/Base/NewestBuffer/NewestBuffer.hpp>
#include <MulNXExtensions/WinExt/WinExt.hpp>
#include <MulNXExtensions/CS2/Signatures.hpp>

#include <MulNXExtensions/CS2/CSClasses/tree/tree.hpp>
#include <MulNXExtensions/CS2/CSClasses/GlobalVars/GlobalVars.hpp>
#include <MulNXExtensions/CS2/CSClasses/CSDll/CSDll.hpp>
#include <MulNXExtensions/CS2/CSClasses/C_CSGameRules/C_CSGameRules.hpp>

#include "ConVarSystem/ConVarSystem.hpp"
#include "FreeCameraController/FreeCameraController.hpp"
#include "AdvancedViewController/AdvancedViewController.hpp"

class Dofs{
public:
    float* pNearBlurry = nullptr;
    float* pNearCrisp = nullptr;
    float* pFarCrisp = nullptr;
    float* pFarBlurry = nullptr;
};

class ControlView {
public:
    std::atomic<bool> hasViewToGame = false;
    MulNX::NewestBuffer<MulNX::Math::View> ViewToGame{};
    MulNX::NewestBuffer<MulNX::Math::View> currentView{};
    std::atomic<float> InputRoll = 0;
    std::atomic<bool> CameraMode = false;
    std::atomic<int> WindowWidth = 1920;
    std::atomic<int> WindowHeight = 1080;
    
    Dofs dofs{};
};

class CSController final :public MulNX::IAbstractLayer3D {
private:
    ControlView controlView{};

    FreeCameraController* pFreeCameraController{};
    AdvancedViewController* pAdvancedViewController = nullptr;

    void* Source2EngineToClient001 = nullptr;
    // 控制台指令执行器
    VExecutor<void(int, const char*, int)> executor{};
    // 控制台变量系统
    C_ConVarSystem CvarSystem{};
    // CS2全局变量
    C_GlobalVars* CSGlobalVars{};
    // 视角控制钩子
    std::unique_ptr<MulNX::Hook> hkPosCallIsPlayingDemo = nullptr;

    bool Window(MulNX::UINode* node);

    void HandleOverrideView(CS2::CViewSetup* viewSetup);
    void HandleCameraSystemPlay(CS2::CViewSetup* viewSetup);

    void EnlistExecutors();
    void ProcessMsg(MulNX::Message& Msg)override;
    void Update();
public:
    std::vector<std::function<bool(CS2::CCSPlayerController*, CS2::C_CSPlayerPawn*)>>handlesControlPlayer{};
    CS2::Modules Modules{};
    // 获取控制台变量系统
    C_ConVarSystem& GetCvarSystem() { return this->CvarSystem; }
    
    bool Init()override;
    
    VExecutor<void* ()> GetDemo{};
    VExecutor<int()>GetDemoTick{};
    VExecutor<bool()>IsPlayingDemo{};
    VExecutor<bool()>IsDemoPaused{};
    

    // AbstractLayer3D接口实现
    float* GetViewMatrix()override;
    MulNX::Math::View GetView()override;
    float GetTime()override;
    bool JumpTime(const float time)override;
    float GetWinWidth()const override;
    float GetWinHeight()const override;
    bool SpecPlayer(int IndexInMap)override;
    D_Player& GetPlayerMsg(int Index)override;
    void spec_goto_ex(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& rot)override;
    void ClearViewOverride()override;
    void SetDOF(const MulNX::Math::DOFParam& dof)override;
    void HandleFreeCameraPath(const CameraSystemIO* const IO);
    bool CameraSystemIOOverride(const CameraSystemIO* const IO)override;
};