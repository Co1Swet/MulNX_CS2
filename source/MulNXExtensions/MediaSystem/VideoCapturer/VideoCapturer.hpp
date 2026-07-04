#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>
#include <MulNXThirdParty/queue/concurrentqueue.h>
#include <vector>

class VideoCapturer final :public MediaModuleBase {
    class VCD3D11Manager* pVCD3D11Manager = nullptr;
    class VEncodeHelper*  pVEncodeHelper  = nullptr;
    moodycamel::ConcurrentQueue<av::VideoFrame> buffer;

    std::optional<std::chrono::steady_clock::time_point> recordStartTime;

    // CPU 读回
    ID3D11Texture2D* pStagingTex = nullptr;
    DXGI_FORMAT stagingFormat = DXGI_FORMAT_UNKNOWN;
    int stagingWidth = 0, stagingHeight = 0;
    av::PixelFormat srcPixelFormat = AV_PIX_FMT_NONE;
    std::vector<uint8_t> readbackBuf;

    bool hwCapture = false;   // 是否走 hw 零拷贝路径

    void ReleaseStagingTexture();
    void Captuer();
    bool ReadbackSw(int slotIdx, int64_t ptsUs);
    bool ReadbackHw(int slotIdx, int64_t ptsUs);

public:
    bool Init()override;
    void Reset();
    void ClearBuffer();
    void StartCapture(const std::chrono::steady_clock::time_point& startTime, bool hwPath);
    void StopCapture();
    std::optional<av::VideoFrame> TryPop();
};
