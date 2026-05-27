#pragma once

#include <MulNX/MulNX.hpp>
#include <MulNXExtensions/GraphicsManager/GraphicsManager.hpp>
#include <MulNXExtensions/MediaSystem/D3D11AV.hpp>
#include <MulNXThirdParty/queue/concurrentqueue.h>

class FrameCapturer final :public MulNX::ModuleBase {
    MulNX::GraphicsManager* pGraphicsManager = nullptr;

    moodycamel::ConcurrentQueue<av::VideoFrame>buffer;

    void ReleaseStagingTexture();
public:
    ID3D11Texture2D* pStagingTex = nullptr;
    DXGI_FORMAT stagingFormat = DXGI_FORMAT_UNKNOWN;
    int stagingWidth = 0;
    int stagingHeight = 0;
    av::PixelFormat srcPixelFormat = AV_PIX_FMT_NONE;
   

    bool Init()override;
    void Reset();

    void Captuer();
    std::optional<av::VideoFrame> TryPop();
};