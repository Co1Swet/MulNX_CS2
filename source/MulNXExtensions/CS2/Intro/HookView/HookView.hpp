#pragma once
#include <MulNX/Base/NewestBuffer/NewestBuffer.hpp>
#include <Feature/View/FreeCameraController/FreeCameraController.hpp>
#include <Feature/View/CameraSystem/CameraSystemIO/CameraSystemIO.hpp>

class Dofs {
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

class ICSViewControlModule;
class HookView final :public CSModuleBase {
    FreeCameraController* pFreeCameraController = nullptr;
    // 视角控制钩子
    std::unique_ptr<MulNX::Hook> hkPosCallIsPlayingDemo = nullptr;
    ControlView controlView{};
    void HandleCameraSystemPlay(CS2::CViewSetup* viewSetup);
    void HandleOverrideView(CS2::CViewSetup* viewSetup);
    void HandleFreeCameraPath(const CameraSystemIO* const IO);

    bool Menu(MulNX::UINode* node);
public:
    bool Init()override;
    std::vector<ICSViewControlModule*>viewControlModules{};
    float GetWinWidth()const;
    float GetWinHeight()const;
    MulNX::Math::View GetView();
    float* GetViewMatrix();

    void spec_goto_ex(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& rot);
    void ClearViewOverride();
    void SetDOF(const MulNX::Math::DOFParam& dof);
    bool CameraSystemIOOverride(const CameraSystemIO* const IO);
};