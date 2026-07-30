#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

class AEncodeHelper final :public MediaModuleBase {
    class AudioCapturer* pAudioCapturer = nullptr;
    av::Stream astream;
    av::AudioEncoderContext aencoder;
    av::AudioResampler aresampler;

    std::deque<av::AudioSamples> audioFifo;

    void Reset();
    void SetOn(const MulNX::AVStartInfo& info);
    bool CheckResampler(av::AudioSamples& converted, av::AudioSamples&& asamples);

    bool Init()override;
public:
    std::optional<av::Packet> TrySetOff();
    moodycamel::ConcurrentQueue<av::AudioSamples> bufferAudioSampleses;
    std::optional<av::Packet> Encode();
};