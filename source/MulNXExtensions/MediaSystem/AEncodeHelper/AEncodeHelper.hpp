#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

class AEncodeHelper final :public MediaModuleBase {
    av::Stream astream;
    av::AudioEncoderContext aencoder;
    av::AudioResampler aresampler;

    std::deque<av::AudioSamples> audioFifo;

    int64_t aptsCounter = 0;
public:
    bool Init()override;

    void SetOn(av::FormatContext* oCtx, int sampleRate);
    std::optional<av::Packet> TrySetOff();
    void Reset();

    bool CheckResampler(av::AudioSamples& converted, av::AudioSamples&& asamples);
    std::optional<av::Packet> Encode(av::AudioSamples&& asamples);
};