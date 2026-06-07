#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

class VCD3D11Manager final :public MediaModuleBase {
    ID3D11Device* pDevice = nullptr;
    ID3D11DeviceContext* pContext = nullptr;

    ID3D11Texture2D* pSharedTex = nullptr;
    HANDLE hSharedHandle = nullptr; // 共享句柄
    ID3D11Texture2D* pRemoteTex = nullptr; // 本设备上打开的接收纹理

    void OnPresentFirst(MulNX::Message& msg);
public:
    bool Init()override;
};