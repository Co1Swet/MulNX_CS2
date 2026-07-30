#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>
#include <MulNXThirdParty/queue/concurrentqueue.h>
#include <vector>

class VideoCapturer final :public MediaModuleBase {
    class VCD3D11Manager* pVCD3D11Manager = nullptr;
    class VEncodeHelper* pVEncodeHelper = nullptr;
    class BufferCopier* pBufferCopier = nullptr;
    class TextureMapper* pTextureMapper = nullptr;

    std::optional<std::chrono::steady_clock::time_point> recordStartTime;
    std::atomic<bool> vCapturing = false;

    bool Init()override;

    void Captuer();
    void Reset();

    void StartCapture(const std::chrono::steady_clock::time_point& startTime);
    void StopCapture();
};