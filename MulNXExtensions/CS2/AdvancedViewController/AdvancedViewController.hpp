#pragma once

#include <MulNX/Base/NewestBuffer/NewestBuffer.hpp>
#include <MulNX/MulNX.hpp>
#include <MulNXExtensions/WinExt/WinExt.hpp>
#include <MulNXExtensions/CS2/CSModuleBase.hpp>

#include <expected>

class AxisInfo {
public:
    DirectX::XMFLOAT3 PosOrigin;
    DirectX::XMFLOAT3 PosForward;
    DirectX::XMFLOAT3 PosUp;

    // 轴向单位向量（局部坐标系的三轴）
    DirectX::XMFLOAT3 AxisForward;
    DirectX::XMFLOAT3 AxisLeft;
    DirectX::XMFLOAT3 AxisUp;
};

class AdvancedViewController final :public CSModuleBase {
private:
    std::atomic<bool> Enable = false;
    std::atomic<bool> OverrideSelfView = false;
    std::atomic<bool> AlwaysCaulate = false;

    std::atomic<bool> useLocalPawn = false;
    // 强制头模式
    std::atomic<bool> forceHeadMode = false;
    // 是否使用人体骨骼而非武器骨骼
    std::atomic<bool> UseBodyBones = false;

    // 骨骼索引
    std::atomic<int> boneIndex1{ 0 };
    std::atomic<int> boneIndex2{ 1 };
    std::atomic<int> boneIndex3{ 2 };

    // 位置和旋转偏移
    DirectX::XMFLOAT3 localPositionOffset = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 localRotationOffset = { 0.0f, 0.0f, 0.0f };

    // 调试信息
    std::atomic<bool> hasAxisInfo = false;
    MulNX::NewestBuffer<AxisInfo> currentAxisInfo;
    // 是否反转上向（用于修正骨骼上向方向）
    std::atomic<bool> InvertUp{ false };
    // 绘制选项
    std::atomic<bool> ShowOriginalBones{ true };
    std::atomic<bool> ShowCoordinateAxes{ true };
    // 绘制坐标轴的长度（世界单位）
    std::atomic<float> AxisLength{ 30.0f };
    MulNX::Math::ViewBuffer viewBuffer{};
public:
    bool Init()override;
    bool Menu(MulNX::UINode* node);

    void HandleUpdate(CS2::CViewSetup* viewSetup);
    CS2::C_CSPlayerPawn* GetSelfViewTargetPawn();
    std::expected<MulNX::Math::Point3, int> GetPoint3(CS2::CViewSetup* viewSetup);
    std::expected<MulNX::Math::View, int> HandleSelfViewUpdate(CS2::CViewSetup* viewSetup);

    bool HandleOverrideView(CS2::CViewSetup* viewSetup);
};