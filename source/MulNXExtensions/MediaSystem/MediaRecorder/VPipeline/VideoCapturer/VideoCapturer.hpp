#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

class VideoCapturer final :public MediaModuleBase {
    class VCD3D11Manager* pVCD3D11Manager = nullptr;
    class VEncodeHelper* pVEncodeHelper = nullptr;
    class TextureMapper* pTextureMapper = nullptr;

    std::atomic<std::chrono::steady_clock::time_point> recordStartTime;

    bool Init()override;

    void Captuer();
    void Reset();

    void StartCapture(const std::chrono::steady_clock::time_point& startTime);
    void StopCapture();
};