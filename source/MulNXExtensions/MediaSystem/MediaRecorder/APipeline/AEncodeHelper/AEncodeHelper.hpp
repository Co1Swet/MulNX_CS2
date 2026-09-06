#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

class AEncodeHelper final : public MediaModuleBase {
    class AudioCapturer* pAudioCapturer = nullptr;

    av::Stream astream;
    av::AudioEncoderContext aencoder;
    av::AudioResampler aresampler;

    std::deque<av::AudioSamples> audioFifo;          // 重采样后等待凑帧的样本块
    std::deque<av::Packet> flushPackets;       // TrySetOff 冲刷出的所有包
    std::atomic<double> lastStreamDuration = 0.0;

    int frameSize = 0;                              // 编码器要求的帧长（采样点数）

    // 槽位计数器：保证 PTS 绝对连续
    bool slotInitialized = false;
    int64_t slotCounter = 0;

    bool Init() override;
    void Reset();
    void SetOn(const MulNX::AVStartInfo& info);
    bool CheckResampler(av::AudioSamples& converted, av::AudioSamples&& asamples);

    std::optional<av::Packet> EncodeOneFrame();
    void FlushAll();
public:
    moodycamel::ConcurrentQueue<av::AudioSamples> bufferAudioSampleses;

    std::optional<av::Packet> Encode();
    std::optional<av::Packet> TrySetOff();
};