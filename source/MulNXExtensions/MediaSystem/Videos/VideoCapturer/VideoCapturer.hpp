#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>
#include <MulNXThirdParty/queue/concurrentqueue.h>
#include <vector>

class VideoCapturer final :public MediaModuleBase {
    class VCD3D11Manager* pVCD3D11Manager = nullptr;
    class VEncodeHelper* pVEncodeHelper = nullptr;
    class BufferCopier* pBufferCopier = nullptr;
    class TextureMapper* pTextureMapper = nullptr;
    moodycamel::ConcurrentQueue<av::VideoFrame> buffer;

    std::optional<std::chrono::steady_clock::time_point> recordStartTime;
    std::atomic<bool> vCapturing = false;

    void Captuer();
    bool Init()override;

    void Reset();
public:
    void StartCapture(const std::chrono::steady_clock::time_point& startTime);
    void StopCapture();
    std::optional<av::VideoFrame> TryPop();
};
