#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

class VideoCapturer final :public MediaModule<VideoCapturer> {
    class VCD3D11Manager* pVCD3D11Manager = nullptr;
    class VEncodeController* pVEncodeController = nullptr;
    class TextureMapper* pTextureMapper = nullptr;

    std::atomic<std::chrono::steady_clock::time_point> recordStartTime;

    bool Init()override;

    void Captuer();

    void SetOn(const std::chrono::steady_clock::time_point& startTime);
    void StopCapture();
};