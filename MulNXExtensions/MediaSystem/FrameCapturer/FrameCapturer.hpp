#pragma once

#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>
#include <MulNXThirdParty/queue/concurrentqueue.h>

class FrameCapturer final :public MediaModuleBase {
    moodycamel::ConcurrentQueue<av::VideoFrame>buffer;

    void ReleaseStagingTexture();
    void Captuer();
public:
    ID3D11Texture2D* pStagingTex = nullptr;
    DXGI_FORMAT stagingFormat = DXGI_FORMAT_UNKNOWN;
    int stagingWidth = 0;
    int stagingHeight = 0;
    av::PixelFormat srcPixelFormat = AV_PIX_FMT_NONE;
   

    bool Init()override;
    void Reset();

    
    std::optional<av::VideoFrame> TryPop();
};