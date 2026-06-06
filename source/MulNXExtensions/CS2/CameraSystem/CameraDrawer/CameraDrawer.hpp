#pragma once

#include <MulNX/Base/UI/UI.hpp>
#include <MulNX/MulNX.hpp>

class CameraModel {
public:

    DirectX::XMFLOAT3 Origin{};
    DirectX::XMFLOAT3 Lens[4]{};
    DirectX::XMFLOAT3 X{};
    DirectX::XMFLOAT3 Y{};
    DirectX::XMFLOAT3 Z{};

    ImU32 Colour{};

    DirectX::XMFLOAT2 Origin2D{};
    DirectX::XMFLOAT2 Lens2D[4]{};
    DirectX::XMFLOAT2 X2D{};
    DirectX::XMFLOAT2 Y2D{};
    DirectX::XMFLOAT2 Z2D{};

    void Init(float CameraHigh, float CameraX, float CameraY, float AxisLenth, ImU32 Colour);

    void Rotate(const DirectX::XMFLOAT3& Rotation);
    void Move(const DirectX::XMFLOAT3& TargetPoint);
    void ToScreen(const float* const pMatrix, const float WinWidth, const float WinHeight);

    void Draw(const char* Lable);

    void DrawCamera(const DirectX::XMFLOAT3& Position, const DirectX::XMFLOAT3& Rotation, const char* label,
        const float* const pMatrix, const float WinWidth, const float WinHeight);

    CameraModel() = default;
};

class CameraDrawer {
private:
    // Model源
    CameraModel CameraModel{};

    float* pMatrixPtr{};
    float WinWidth{};
    float WinHeight{};
public:
    void Init(float CameraHigh, float CameraX, float CameraY, float AxisLenth, ImU32 Colour);

    void Update(float* pMatrixPtr, float pWinWidth, float pWinHeight);
    void DrawCamera(DirectX::XMFLOAT3 Position, DirectX::XMFLOAT3 Rotation, const char* label);
    void DrawFrameCamera(const MulNX::Math::Frame& frame, const char* label);
};