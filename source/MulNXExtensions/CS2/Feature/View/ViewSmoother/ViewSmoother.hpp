#pragma once
#include <Intro/HookView/CSViewControlModuleBase.hpp>

class ViewBuffer {
    MulNX::Math::View view;
public:
    // 平滑系数
    std::atomic<float> smooth_buffer{ 0.2f };
    void Push(MulNX::Math::View&& newView);
    MulNX::Math::View& Get() { return this->view; }
};

class ViewSmoother final :public CSModuleBase, public CSViewControlMixin<ViewSmoother> {
    std::atomic<bool> enable = false;
    ViewBuffer buffer;
    void Menu();
    bool HandleUpdateCSView(CS2::CViewSetup* viewSetup, const int& num, bool& camLeavePlayer)override;
    bool Init()override;
};