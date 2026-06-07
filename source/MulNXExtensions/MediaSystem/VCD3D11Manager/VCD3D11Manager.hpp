#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

class MidTex {
public:
    ComPtr<ID3D11Texture2D> pTex;
    ComPtr<IDXGIKeyedMutex> pMutex;
};

class DoubleInterfaceTex {
public:
    MidTex srcTex; // 原设备上的共享纹理和同步接口
    MidTex dstTex; // 录制设备上的共享纹理和同步接口
};

class VCD3D11Manager final : public MediaModuleBase {
    ComPtr<ID3D11Device> pDevice;
    ComPtr<ID3D11DeviceContext> pContext;

    DoubleInterfaceTex buffer1;

    void OnPresentFirst(MulNX::Message& msg);
    void CopyTexture();
public:
    bool Init() override;
};