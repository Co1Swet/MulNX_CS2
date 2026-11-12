#include "AudioModify.hpp"

uint64_t DefaultChannelLayout(int channels) {
    if (channels == 1) {
        return AV_CH_LAYOUT_MONO;
    }
    if (channels == 2) {
        return AV_CH_LAYOUT_STEREO;
    }

    AVChannelLayout layout{};
    av_channel_layout_default(&layout, channels);
    const uint64_t mask = layout.order == AV_CHANNEL_ORDER_NATIVE ? layout.u.mask : 0;
    av_channel_layout_uninit(&layout);
    return mask;
}

int64_t GetDurationSamples(const av::Timestamp& duration, int sampleRate) {
    if (!duration.isValid() || duration.seconds() <= 0.0) {
        return 0;
    }
    return static_cast<int64_t>(std::llround(duration.seconds() * sampleRate));
}

int64_t GetVideoDurationUs(av::FormatContext& context, const av::Stream& videoStream) {
    const auto streamDuration = videoStream.duration();
    if (streamDuration.isValid() && streamDuration.seconds() > 0.0) {
        return streamDuration.timestamp(MicrosecondTimeBase);
    }

    const auto formatDuration = context.duration();
    if (formatDuration.isValid() && formatDuration.seconds() > 0.0) {
        return formatDuration.timestamp(MicrosecondTimeBase);
    }

    throw std::runtime_error("无法确定视频流时长，不能对齐音频");
}

AudioEncoderConfig CreateAudioEncoderConfig(const av::Stream& audioStream,
    const std::string& encoderName) {
    av::AudioDecoderContext decoder(audioStream);
    decoder.open();

    auto encoderCodec = av::findEncodingCodec(encoderName);
    if (!encoderCodec.canEncode()) {
        throw std::runtime_error("指定的音频编码器不可用: " + encoderName);
    }

    const int sourceRate = decoder.sampleRate() > 0 ? decoder.sampleRate() : 48000;
    const int sourceChannels = decoder.channels() > 0 ? decoder.channels() : 2;
    int outputRate = sourceRate;
    auto sampleRates = encoderCodec.supportedSamplerates();
    if (!sampleRates.empty() &&
        std::find(sampleRates.begin(), sampleRates.end(), outputRate) == sampleRates.end()) {
        outputRate = *std::min_element(sampleRates.begin(), sampleRates.end(),
            [sourceRate](int left, int right) {
                return std::abs(left - sourceRate) < std::abs(right - sourceRate);
            });
    }

    uint64_t sourceLayout = decoder.channelLayout();
    if (!sourceLayout) {
        sourceLayout = DefaultChannelLayout(sourceChannels);
    }

    auto layouts = encoderCodec.supportedChannelLayouts();
    uint64_t outputLayout = sourceLayout;
    if (!layouts.empty() && std::find(layouts.begin(), layouts.end(), sourceLayout) == layouts.end()) {
        outputLayout = layouts.front();
    }
    if (!outputLayout) {
        outputLayout = AV_CH_LAYOUT_STEREO;
    }

    auto formats = encoderCodec.supportedSampleFormats();
    if (formats.empty()) {
        throw std::runtime_error("音频编码器没有报告支持的样本格式");
    }
    const auto sourceFormat = decoder.sampleFormat();
    const auto formatIt = std::find(formats.begin(), formats.end(), sourceFormat);
    const auto outputFormat = formatIt != formats.end() ? *formatIt : formats.front();
    const int outputChannels = static_cast<int>(std::popcount(outputLayout));
    if (outputChannels <= 0) {
        throw std::runtime_error("无法确定音频编码器的声道数");
    }

    return { encoderCodec, outputRate, outputChannels, outputLayout, outputFormat };
}

av::AudioEncoderContext CreateAudioEncoder(const AudioEncoderConfig& config) {
    av::AudioEncoderContext encoder(config.codec);
    encoder.setSampleRate(config.sampleRate);
    encoder.setChannels(config.channels);
    encoder.setChannelLayout(config.channelLayout);
    encoder.setSampleFormat(config.sampleFormat);
    encoder.setTimeBase({ 1, config.sampleRate });
    if (config.codec.id() == AV_CODEC_ID_MP3) {
        encoder.setBitRate(192000);
    }
    encoder.open();
    return encoder;
}


AudioSegmentWriter::AudioSegmentWriter(av::FormatContext& output, const av::Stream& outputStream,
    av::AudioDecoderContext* decoder, const AudioEncoderConfig& config,
    int64_t targetSamples, int64_t timelineOffsetSamples)
    : output_(output),
    outputStream_(outputStream),
    decoder_(decoder),
    config_(config),
    encoder_(CreateAudioEncoder(config)),
    targetSamples_(targetSamples),
    timelineOffsetSamples_(timelineOffsetSamples),
    frameSize_(std::max(encoder_.frameSize(), 1024)) {
    AllocateFrame();
}

void AudioSegmentWriter::ProcessPacket(const av::Packet& packet) {
    if (!decoder_) {
        return;
    }
    auto samples = decoder_->decode(packet);
    ProcessDecodedSamples(samples);
}

void AudioSegmentWriter::Finish() {
    if (decoder_) {
        ProcessDecodedSamples(decoder_->decode(av::Packet()));
    }
    FlushResampler();

    while (acceptedSamples_ < targetSamples_) {
        const int available = static_cast<int>(std::min<int64_t>(
            frameSize_ - frameFill_, targetSamples_ - acceptedSamples_));
        frameFill_ += available;
        acceptedSamples_ += available;
        if (frameFill_ == frameSize_) {
            EncodeFrame();
        }
    }
    if (frameFill_ > 0) {
        EncodeFrame();
    }

    while (true) {
        auto packet = encoder_.encode();
        if (!packet || packet.isNull() || packet.size() == 0) {
            break;
        }
        WriteEncodedPacket(packet);
    }
}

void AudioSegmentWriter::CopySamples(av::AudioSamples& destination, int destinationOffset,
    const av::AudioSamples& source, int sourceOffset, int sampleCount) {
    const size_t bytesPerSample = source.sampleFormat().bytesPerSample();
    if (source.isPlanar()) {
        for (int channel = 0; channel < source.channelsCount(); ++channel) {
            std::memcpy(
                destination.data(channel) + static_cast<size_t>(destinationOffset) * bytesPerSample,
                source.data(channel) + static_cast<size_t>(sourceOffset) * bytesPerSample,
                static_cast<size_t>(sampleCount) * bytesPerSample);
        }
        return;
    }

    const size_t bytesPerFrame = bytesPerSample * static_cast<size_t>(source.channelsCount());
    std::memcpy(
        destination.data(0) + static_cast<size_t>(destinationOffset) * bytesPerFrame,
        source.data(0) + static_cast<size_t>(sourceOffset) * bytesPerFrame,
        static_cast<size_t>(sampleCount) * bytesPerFrame);
}

void AudioSegmentWriter::AllocateFrame() {
    frame_ = av::AudioSamples();
    if (frame_.init(config_.sampleFormat, frameSize_, config_.channelLayout,
        config_.sampleRate) < 0) {
        throw std::runtime_error("无法分配音频编码帧");
    }
    const int planes = frame_.isPlanar() ? frame_.channelsCount() : 1;
    for (int plane = 0; plane < planes; ++plane) {
        std::memset(frame_.data(plane), 0, frame_.size(plane));
    }
    frameFill_ = 0;
}

void AudioSegmentWriter::ProcessDecodedSamples(const av::AudioSamples& samples) {
    if (!samples.isValid() || samples.samplesCount() <= 0 || acceptedSamples_ >= targetSamples_) {
        return;
    }

    const bool sameFormat =
        samples.sampleFormat() == config_.sampleFormat &&
        samples.sampleRate() == config_.sampleRate &&
        samples.channelsLayout() == config_.channelLayout;
    if (sameFormat) {
        AppendSamples(samples);
        return;
    }

    uint64_t sourceLayout = samples.channelsLayout();
    if (!sourceLayout) {
        sourceLayout = DefaultChannelLayout(samples.channelsCount());
    }
    if (!resampler_.isValid()) {
        if (!resampler_.init(config_.channelLayout, config_.sampleRate, config_.sampleFormat,
            sourceLayout, samples.sampleRate(), samples.sampleFormat())) {
            throw std::runtime_error("初始化音频重采样器失败");
        }
    }
    else if (resampler_.srcSampleRate() != samples.sampleRate() ||
        resampler_.srcSampleFormat() != samples.sampleFormat() ||
        resampler_.srcChannels() != samples.channelsCount() ||
        resampler_.srcChannelLayout() != sourceLayout) {
        throw std::runtime_error("同一音频流的格式在处理中发生变化");
    }

    resampler_.push(samples);
    AppendResampledSamples();
}

void AudioSegmentWriter::AppendResampledSamples() {
    auto samples = resampler_.pop(0);
    if (samples.isValid() && samples.samplesCount() > 0) {
        AppendSamples(samples);
    }
}

void AudioSegmentWriter::FlushResampler() {
    if (!resampler_.isValid()) {
        return;
    }
    resampler_.push(av::AudioSamples());
    AppendResampledSamples();
}

void AudioSegmentWriter::AppendSamples(const av::AudioSamples& samples) {
    int sourceOffset = 0;
    while (sourceOffset < samples.samplesCount() && acceptedSamples_ < targetSamples_) {
        const int take = static_cast<int>(std::min<int64_t>(
            std::min(frameSize_ - frameFill_, samples.samplesCount() - sourceOffset),
            targetSamples_ - acceptedSamples_));
        CopySamples(frame_, frameFill_, samples, sourceOffset, take);
        frameFill_ += take;
        sourceOffset += take;
        acceptedSamples_ += take;
        if (frameFill_ == frameSize_) {
            EncodeFrame();
        }
    }
}

void AudioSegmentWriter::EncodeFrame() {
    frame_.setTimeBase({ 1, config_.sampleRate });
    frame_.setPts(av::Timestamp(submittedSamples_, { 1, config_.sampleRate }));
    submittedSamples_ += frameSize_;

    auto packet = encoder_.encode(frame_);
    if (packet && !packet.isNull() && packet.size() > 0) {
        WriteEncodedPacket(packet);
    }
    AllocateFrame();
}

void AudioSegmentWriter::WriteEncodedPacket(av::Packet& packet) {
    const int64_t localStart = emittedSamples_;
    emittedSamples_ += frameSize_;
    if (localStart >= targetSamples_) {
        return;
    }

    const int64_t packetDuration = std::min<int64_t>(frameSize_, targetSamples_ - localStart);
    const int64_t pts = timelineOffsetSamples_ + localStart;
    packet.setPts(av::Timestamp(pts, { 1, config_.sampleRate }));
    packet.setDts(av::Timestamp(pts, { 1, config_.sampleRate }));
    packet.setDuration(static_cast<int>(packetDuration), { 1, config_.sampleRate });
    packet.setStreamIndex(outputStream_.index());
    packet.setTimeBase({ 1, config_.sampleRate });
    output_.writePacket(packet);
}