#pragma once
#include <MulNX/Base/NewestBuffer/NewestBuffer.hpp>
#include <Intro/CSModuleBase.hpp>

class Dofs {
public:
    float* pNearBlurry = nullptr;
    float* pNearCrisp = nullptr;
    float* pFarCrisp = nullptr;
    float* pFarBlurry = nullptr;
};

class ControlView {
public:
    MulNX::NewestBuffer<MulNX::Math::View> currentView{};
    std::atomic<float> InputRoll = 0;
    std::atomic<int> WindowWidth = 1920;
    std::atomic<int> WindowHeight = 1080;

    Dofs dofs{};
};

class ICSViewControlModule;
class HookView final :public CSModuleBase {
    // 视角控制钩子
    std::unique_ptr<MulNX::Hook> hkPosCallIsPlayingDemo{};
    std::atomic<bool> cameraLeavePlayer = false;
    ControlView controlView{};
    void HandleOverrideView(CS2::CViewSetup* viewSetup);

    void Window(MulNX::UICoordinator* uico);
    bool Init()override;
public:
    std::vector<ICSViewControlModule*>viewControlModules{};
    float GetWinWidth()const;
    float GetWinHeight()const;
    MulNX::Math::View GetView();
    float* GetViewMatrix();

    void spec_goto_ex(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& rot);
    void SetDOF(const MulNX::Math::DOFParam& dof);

    inline bool GetCameraLeavePlayerState() { return this->cameraLeavePlayer.load(std::memory_order_acquire); }
};