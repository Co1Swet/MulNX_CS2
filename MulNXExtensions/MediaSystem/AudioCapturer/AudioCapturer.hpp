#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>
#include <audioclient.h>
#include <Windows.h>

class AudioCapturer final :public MediaModuleBase {
private:
    moodycamel::ConcurrentQueue<av::AudioSamples> buffer;
    std::atomic<bool> capturing{ false };
    // no dedicated std::thread; Main is scheduled via ISys task
    Microsoft::WRL::ComPtr<IAudioCaptureClient> captureClient;
    Microsoft::WRL::ComPtr<IAudioClient> audioClient;
    WAVEFORMATEX* wfx = nullptr;
    HANDLE hEvent = nullptr;

    void Main();
public:
    bool Init()override;
    void Deinit()override;

    std::optional<av::AudioSamples> TryPop();
};
