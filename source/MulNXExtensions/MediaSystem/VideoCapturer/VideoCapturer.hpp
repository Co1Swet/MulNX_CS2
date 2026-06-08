#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>
#include <MulNXThirdParty/queue/concurrentqueue.h>

class VideoCapturer final :public MediaModuleBase {
    class VCD3D11Manager* pVCD3D11Manager = nullptr;
    moodycamel::ConcurrentQueue<av::VideoFrame>buffer;
    
    std::optional<std::chrono::steady_clock::time_point> recordStartTime;

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
    void ClearBuffer();
    void StartCapture(const std::chrono::steady_clock::time_point& startTime);
    void StopCapture();

    std::optional<av::VideoFrame> TryPop();
};