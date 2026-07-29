#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>
#include <audioclient.h>

class AudioCapturer final :public MediaModuleBase {
    moodycamel::ConcurrentQueue<av::AudioSamples> buffer;
    std::atomic<bool> keepWork = false;

    Microsoft::WRL::ComPtr<IAudioCaptureClient> captureClient;
    Microsoft::WRL::ComPtr<IAudioClient> audioClient;
    WAVEFORMATEX* wfx = nullptr;
    HANDLE hEvent = nullptr;

    void Main();

    bool Init()override;
    void Deinit()override;

    void ClearBuffer();
public:
    std::optional<av::AudioSamples> TryPop();
    
    // audio info accessors
    WAVEFORMATEX* GetWfx() const { return wfx; }
    int GetSampleRate() const { return wfx ? wfx->nSamplesPerSec : 0; }
    int GetChannels() const { return wfx ? wfx->nChannels : 0; }
};
