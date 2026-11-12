#pragma once
#include <avcpp/rational.h>
#include <avcpp/codec.h>
#include <avcpp/sampleformat.h>
#include <avcpp/formatcontext.h>
#include <avcpp/codeccontext.h>
#include <avcpp/audioresampler.h>
#include <string>

const av::Rational MicrosecondTimeBase{ 1, 1000000 };
struct AudioEncoderConfig {
    av::Codec codec;
    int sampleRate = 0;
    int channels = 0;
    uint64_t channelLayout = 0;
    av::SampleFormat sampleFormat;
};

uint64_t DefaultChannelLayout(int channels);
int64_t GetDurationSamples(const av::Timestamp& duration, int sampleRate);
int64_t GetVideoDurationUs(av::FormatContext& context, const av::Stream& videoStream);

AudioEncoderConfig CreateAudioEncoderConfig(const av::Stream& audioStream,
    const std::string& encoderName);
av::AudioEncoderContext CreateAudioEncoder(const AudioEncoderConfig& config);

class AudioSegmentWriter {
    av::FormatContext& output_;
    av::Stream outputStream_;
    av::AudioDecoderContext* decoder_;
    AudioEncoderConfig config_;
    av::AudioEncoderContext encoder_;
    av::AudioResampler resampler_;
    av::AudioSamples frame_;
    int64_t targetSamples_ = 0;
    int64_t timelineOffsetSamples_ = 0;
    int64_t acceptedSamples_ = 0;
    int64_t submittedSamples_ = 0;
    int64_t emittedSamples_ = 0;
    int frameSize_ = 0;
    int frameFill_ = 0;

    static void CopySamples(av::AudioSamples& destination, int destinationOffset,
        const av::AudioSamples& source, int sourceOffset, int sampleCount);
    void AllocateFrame();
    void ProcessDecodedSamples(const av::AudioSamples& samples);
    void AppendResampledSamples();
    void FlushResampler();
    void AppendSamples(const av::AudioSamples& samples);
    void EncodeFrame();
    void WriteEncodedPacket(av::Packet& packet);
public:
    AudioSegmentWriter(av::FormatContext& output, const av::Stream& outputStream,
        av::AudioDecoderContext* decoder, const AudioEncoderConfig& config,
        int64_t targetSamples, int64_t timelineOffsetSamples);

    void ProcessPacket(const av::Packet& packet);
    void Finish();
};