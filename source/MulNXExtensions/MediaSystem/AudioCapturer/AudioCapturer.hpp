#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>
#include <audioclient.h>
#include <Windows.h>
#include <optional>

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

    bool Init()override;
    void Deinit()override;
public:
    std::optional<av::AudioSamples> TryPop();
    void ClearBuffer();
    // audio info accessors
    WAVEFORMATEX* GetWfx() const { return wfx; }
    int GetSampleRate() const { return wfx ? wfx->nSamplesPerSec : 0; }
    int GetChannels() const { return wfx ? wfx->nChannels : 0; }
};
