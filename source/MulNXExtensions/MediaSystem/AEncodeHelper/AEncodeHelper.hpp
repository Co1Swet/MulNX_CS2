#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

class AudioCapturer;
class AEncodeHelper final : public MediaModuleBase {
    class AudioCapturer* pAudioCapturer = nullptr;

    std::atomic<double> lastStreamDuration = 0.0;

    av::Stream          astream;
    av::AudioEncoderContext aencoder;
    av::AudioResampler  aresampler;

    std::deque<av::AudioSamples> audioFifo;          // 重采样后等待凑帧的样本块
    std::deque<av::Packet>       flushPackets;       // TrySetOff 冲刷出的所有包

    int  frameSize = 0;                              // 编码器要求的帧长（采样点数）

    // 槽位计数器：保证 PTS 绝对连续
    int64_t m_slotCounter = 0;
    bool    m_slotInitialized = false;

    void Reset();
    void SetOn(const MulNX::AVStartInfo& info);
    bool CheckResampler(av::AudioSamples& converted, av::AudioSamples&& asamples);

    std::optional<av::Packet> encodeOneFrame();
    void flushAll();

public:
    moodycamel::ConcurrentQueue<av::AudioSamples> bufferAudioSampleses;

    std::optional<av::Packet> Encode();
    std::optional<av::Packet> TrySetOff();

    bool Init() override;
};