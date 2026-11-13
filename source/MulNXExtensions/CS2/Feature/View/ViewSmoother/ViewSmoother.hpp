#pragma once
#include <Intro/HookView/CSViewControlModuleBase.hpp>
#include <atomic>

class ViewBuffer {
    MulNX::Math::View view;
public:
    // 平滑系数
    std::atomic<float> smooth_buffer{ 0.2f };
    void Push(MulNX::Math::View&& newView);
    MulNX::Math::View& Get() { return this->view; }
};

class ViewSmoother final :public CSModule<ViewSmoother>, public CSViewControlMixin<ViewSmoother> {
    MulReflBool(enable, false);
    ViewBuffer buffer;
    std::atomic<float> fov_smooth_buffer{ 1.0f };
    // FOV 历史及初始化状态仅在视图回调线程中访问。
    float smoothed_fov = 0.0f;
    bool fov_initialized = false;
    void Menu();
    bool HandleUpdateCSView(CS2::CViewSetup* viewSetup, const int& num, bool& camLeavePlayer)override;
    bool Init()override;
};
