#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>
#include <MulNXExtensions/MediaSystem/Videos/VCD3D11Manager/VCD3D11Manager.hpp>

class TextureMapper final :public MediaModuleBase {
    VCD3D11Manager* pVCD3D11Manager = nullptr;
    // CPU 读回
    ID3D11Texture2D* pStagingTex = nullptr;
    DXGI_FORMAT stagingFormat = DXGI_FORMAT_UNKNOWN;
    int stagingWidth = 0, stagingHeight = 0;
    av::PixelFormat srcPixelFormat = AV_PIX_FMT_NONE;
    std::vector<uint8_t> readbackBuf;
    std::atomic<bool> nextFrameRelease = false;

    bool Init()override;
    void ReleaseStagingTexture();
    bool CheckStagingTexture(D3D11_TEXTURE2D_DESC& desc);
public:
    std::optional<av::VideoFrame> MapFrame(MidTex& tex);
};