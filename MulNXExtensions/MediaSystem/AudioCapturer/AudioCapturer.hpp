#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

class AudioCapturer final :public MediaModuleBase {
public:
    bool Init()override;
    void Deinit()override;

    std::optional<av::AudioSamples> TryPop();

private:
    moodycamel::ConcurrentQueue<av::AudioSamples> buffer;
    std::atomic<bool> capturing{ false };
    std::unique_ptr<std::thread> captureThread;
};
