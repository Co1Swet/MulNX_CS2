#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

class MidTex {
public:
    ComPtr<ID3D11Texture2D> pTex;
    ComPtr<IDXGIKeyedMutex> pMutex;
};

class DoubleInterfaceTex {
public:
    std::atomic<std::chrono::steady_clock::time_point> captureTime;
    std::atomic<bool> hasNewFrame{ false };
    static_assert(std::atomic<std::chrono::steady_clock::time_point>::is_always_lock_free, "captureTime must be lock-free");
    MidTex rawTex; // 原设备上的共享纹理和同步接口
    MidTex shareTex; // 录制设备上的共享纹理和同步接口
};

class VCD3D11Manager final : public MediaModuleBase {
    MulNX::GraphicsManager* pGraphicsManager = nullptr;

    std::optional<std::chrono::steady_clock::time_point> lastCapture;

    void OnPresentFirst(MulNX::Message& msg);
    void CopyTexture();
public:
    bool Init() override;
    DoubleInterfaceTex buffer1;
    ComPtr<ID3D11Device> pDevice;
    ComPtr<ID3D11DeviceContext> pContext;
};