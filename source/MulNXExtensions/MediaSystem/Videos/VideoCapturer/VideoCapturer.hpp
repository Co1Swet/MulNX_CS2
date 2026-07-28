#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>
#include <MulNXThirdParty/queue/concurrentqueue.h>
#include <vector>

class VideoCapturer final :public MediaModuleBase {
    class VCD3D11Manager* pVCD3D11Manager = nullptr;
    class VEncodeHelper* pVEncodeHelper = nullptr;
    class BufferCopier* pBufferCopier = nullptr;
    moodycamel::ConcurrentQueue<av::VideoFrame> buffer;

    std::optional<std::chrono::steady_clock::time_point> recordStartTime;
    std::atomic<bool> vCapturing = false;

    // CPU 读回
    ID3D11Texture2D* pStagingTex = nullptr;
    DXGI_FORMAT stagingFormat = DXGI_FORMAT_UNKNOWN;
    int stagingWidth = 0, stagingHeight = 0;
    av::PixelFormat srcPixelFormat = AV_PIX_FMT_NONE;
    std::vector<uint8_t> readbackBuf;

    void ReleaseStagingTexture();
    void Captuer();
    bool ReadbackSw(int slotIdx, int64_t ptsUs);

    bool Init()override;
public:
    void Reset();
    void ClearBuffer();
    void StartCapture(const std::chrono::steady_clock::time_point& startTime);
    void StopCapture();
    std::optional<av::VideoFrame> TryPop();
};
