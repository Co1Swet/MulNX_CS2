#pragma once
#include <Intro/CSModuleBase.hpp>
#include <Intro/HookView/CSViewControlModuleBase.hpp>

// 自由摄像机位置控制器（仅位置控制）
class FreeCameraController final :public CSModuleBase, public CSViewControlMixin<FreeCameraController> {
    DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f }; // 位置
    DirectX::XMFLOAT3 Rotation = { 0.0f, 0.0f, 0.0f }; // 旋转角度 (pitch, yaw, roll)
    std::atomic<float> MoveSpeed = 100.0f; // 移动速度 (单位/秒)
    std::atomic<bool> EnableControl = false;

    MulNX::KeyCheckPack kMovUp;
    MulNX::KeyCheckPack kMovDown;

    std::chrono::steady_clock::time_point LastUpdateTime = std::chrono::steady_clock::now();
    void Menu();
    bool Init()override;
    void ProcessMsg(MulNX::Message& msg)override;

    bool HandleUpdate(CS2::CViewSetup* viewSetup, const int& num)override;
};