#pragma once
#include <Intro/CSModuleBase.hpp>

class ViewModelController final :public CSModuleBase {
    std::unique_ptr<MulNX::Hook>hkGetViewModelInfo = nullptr;
    std::unique_ptr<MulNX::Hook>hkGetIfHandLeftSide = nullptr;

    std::atomic<bool>enableX = false;
    std::atomic<float>offsetX = 0.0f;
    std::atomic<bool>enableY = false;
    std::atomic<float>offsetY = 0.0f;
    std::atomic<bool>enableZ = false;
    std::atomic<float>offsetZ = 0.0f;

    std::atomic<bool>enableFov = false;
    std::atomic<float>myFov = 60;

    std::atomic<bool>enableHandSide = false;
    std::atomic<bool>isLeftHandSide = false;

    void Menu();
    void MenuPlayer(MulNX::Message* umsg);
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg);
};