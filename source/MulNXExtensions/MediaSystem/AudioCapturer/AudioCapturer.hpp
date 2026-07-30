#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>
#include <audioclient.h>

class AudioCapturer final :public MediaModuleBase {
    class AEncodeHelper* pAEncodeHelper = nullptr;
    
    std::atomic<bool> keepWork = false;

    Microsoft::WRL::ComPtr<IAudioCaptureClient> captureClient;
    Microsoft::WRL::ComPtr<IAudioClient> audioClient;
    WAVEFORMATEX* wfx = nullptr;
    HANDLE hEvent = nullptr;

    void Main();

    bool Init()override;
    void Deinit()override;
public:    
    int GetSampleRate() const { return wfx ? wfx->nSamplesPerSec : 0; }
};