#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>
#include <audioclient.h>

class AudioCapturer final :public MediaModuleBase {
    class AEncodeHelper* pAEncodeHelper = nullptr;
    
    std::atomic<bool> exitFlag = false;
    std::atomic<bool> needCaptuer = false;

    std::atomic<std::chrono::steady_clock::time_point> recordStartTime;
    std::atomic<std::chrono::steady_clock::time_point> lastCommitTime;

    Microsoft::WRL::ComPtr<IAudioCaptureClient> captureClient;
    Microsoft::WRL::ComPtr<IAudioClient> audioClient;
    WAVEFORMATEX* wfx = nullptr;
    HANDLE hEvent = nullptr;

    void Main();
    void ProcessAudio(const int& samplesCount, const uint64_t& chLayout, std::vector<uint8_t>&& copied);
    void CommitAudioSamples(av::AudioSamples&& samples);

    void OnFirstPresent();

    bool Init()override;
    void Deinit()override;
public:    
    int GetSampleRate() const { return wfx ? wfx->nSamplesPerSec : 0; }
};