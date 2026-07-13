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
    ViewBuffer buffer;
    void Menu();
    bool HandleUpdate(CS2::CViewSetup* viewSetup, const int& num)override;
    bool Init()override;
};