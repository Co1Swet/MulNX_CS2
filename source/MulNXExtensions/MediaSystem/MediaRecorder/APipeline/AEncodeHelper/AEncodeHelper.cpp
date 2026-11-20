#include "AEncodeHelper.hpp"

constexpr int kAudioEncoderSampleRate = 48000;

void CopyAudioSamples(av::AudioSamples& destination, int destinationOffset,
    const av::AudioSamples& source, int sourceOffset, int sampleCount) {
    if (!source.isPlanar()) {
        const int bytesPerSample = source.sampleFormat().bytesPerSample();
        const int channels = source.channelsCount();
        std::memcpy(
            reinterpret_cast<uint8_t*>(destination.data(0)) + destinationOffset * channels * bytesPerSample,
            reinterpret_cast<const uint8_t*>(source.data(0)) + sourceOffset * channels * bytesPerSample,
            static_cast<size_t>(sampleCount) * channels * bytesPerSample);
        return;
    }

    const int bytesPerSample = source.sampleFormat().bytesPerSample();
    for (int channel = 0; channel < source.channelsCount(); ++channel) {
        std::memcpy(
            reinterpret_cast<uint8_t*>(destination.data(channel)) + destinationOffset * bytesPerSample,
            reinterpret_cast<const uint8_t*>(source.data(channel)) + sourceOffset * bytesPerSample,
            static_cast<size_t>(sampleCount) * bytesPerSample);
    }
}

bool AEncodeHelper::Init() {
    this->SubscribeSync("MediaSync/SetOn", [this](MulNX::Message& msg) {
        auto&& [info] = msg.Access<MulNX::AVStartInfo>();
        try {
            this->SetOn(info);
        }
        catch (const std::exception& e) {
            this->LogError(std::format("音频编码SetOn失败：{}", e.what()));
        }
        });
    this->SubscribeSync("MediaSync/StateReport", [this](auto&&...) {
        this->LogInfo(std::format("音频编码器状态: {}", (this->aencoder.isOpened() ? "已打开" : "未打开")));
        this->LogInfo(std::format("音频流状态: {}", (this->astream.isValid() ? "有效" : "无效")));
        this->LogInfo(std::format("音频流总时长: {}秒", this->lastStreamDuration.load()));
        });
    return true;
}

void AEncodeHelper::SetOn(const MulNX::AVStartInfo& info) {
    av::AudioSamples temp;
    while (this->pendingAudioSamples.try_dequeue(temp)) {}
    this->audioFifo.clear();
    this->flushPackets.clear();
    if (this->aencoder.isValid() && this->aencoder.isOpened()) this->aencoder.close();
    this->aencoder = {};
    this->astream = av::Stream();
    this->aresampler = {};
    this->frameSize = 0;
    this->lastStreamDuration = 0.0;
    // 重置槽位计数器
    this->slotInitialized = false;
    this->slotCounter = 0;

    if (info.startAsAdvancedMode) {
        this->LogInfo("高级录制模式，不打开音频捕获");
        return;
    }
    this->audioFifo.clear();
    this->flushPackets.clear();
    av::Codec acodec = av::findEncodingCodec(AV_CODEC_ID_MP3);
    if (!acodec.canEncode()) {
        this->LogError("编码器不可用，仅录制视频");
        return;
    }
    auto supportedFmts = acodec.supportedSampleFormats();
    if (supportedFmts.empty()) {
        this->LogError("编码器未报告支持的样本格式，跳过音频");
        return;
    }
    av::SampleFormat targetFmt = supportedFmts.front();
    this->LogWarning(std::format("音频编码器目标格式: {}", targetFmt.name()));

    this->aencoder = av::AudioEncoderContext(acodec);
    this->aencoder.setSampleRate(kAudioEncoderSampleRate);
    this->aencoder.setChannels(2);
    this->aencoder.setSampleFormat(targetFmt);
    this->aencoder.setChannelLayout(AV_CH_LAYOUT_STEREO);
    this->aencoder.setTimeBase({ 1, kAudioEncoderSampleRate });
    this->aencoder.setBitRate(128000);

    try {
        this->aencoder.open();
        this->frameSize = this->aencoder.frameSize();
        this->LogWarning(std::format("音频编码器已打开，frame_size={} , time_base={} , sample_rate={}",
            this->frameSize, this->aencoder.timeBase(), this->aencoder.sampleRate()));
    }
    catch (const std::exception& e) {
        this->LogError(std::format("音频编码器打开失败: {}", e.what()));
        this->aencoder = {};
        return;
    }

    this->astream = info.pOutCtx->addStream(this->aencoder);
    this->astream.setTimeBase({ 1, kAudioEncoderSampleRate });
    av::CodecParameters cp;
    cp.copyFrom(this->aencoder);
    this->astream.setCodecParameters(cp);

    this->LogInfo(std::format("音频开启={}", this->aencoder.isOpened()));
}

bool AEncodeHelper::CheckResampler(av::AudioSamples& converted, av::AudioSamples&& asamples) {
    int64_t originalUs = asamples.pts().timestamp({ 1, 1000000 });
    bool needResample = (asamples.sampleFormat() != this->aencoder.sampleFormat()) ||
        (asamples.sampleRate() != this->aencoder.sampleRate()) ||
        (asamples.channelsLayout() != this->aencoder.channelLayout());
    if (needResample) {
        if (!this->aresampler.isValid() ||
            this->aresampler.srcSampleRate() != asamples.sampleRate() ||
            this->aresampler.srcSampleFormat() != asamples.sampleFormat() ||
            this->aresampler.srcChannels() != asamples.channelsCount() ||
            this->aresampler.srcChannelLayout() != asamples.channelsLayout()) {
            std::error_code err;
            bool ok = this->aresampler.init(
                this->aencoder.channelLayout(), this->aencoder.sampleRate(), this->aencoder.sampleFormat(),
                asamples.channelsLayout(), asamples.sampleRate(), asamples.sampleFormat(), err);
            if (!ok) {
                this->LogError(std::format("AudioResampler 初始化失败: {}", err ? err.message() : "unknown"));
                return false;
            }
        }
        this->aresampler.push(asamples);
        converted = this->aresampler.pop(0);
        if (!converted.isValid() || converted.samplesCount() == 0) return false;
    }
    else {
        converted = std::move(asamples);
    }
    converted.setTimeBase({ 1, 1000000 });
    converted.setPts(av::Timestamp(originalUs, { 1, 1000000 }));
    return true;
}

void AEncodeHelper::SetPacketTiming(av::Packet& packet, int64_t ptsSamples) {
    const av::Rational sampleTimeBase(1, this->aencoder.sampleRate());
    packet.setStreamIndex(this->astream.index());
    packet.setTimeBase(this->astream.timeBase());
    packet.setPts(av::Timestamp(ptsSamples, sampleTimeBase));
    packet.setDuration(this->frameSize, sampleTimeBase);
}

void AEncodeHelper::SubmitAudioSamples(av::AudioSamples&& samples) {
    this->pendingAudioSamples.enqueue(std::move(samples));
}

std::optional<av::Packet> AEncodeHelper::EncodeOneFrame() {
    int64_t totalSamples = 0;
    for (const auto& buf : this->audioFifo) totalSamples += buf.samplesCount();
    if (totalSamples < this->frameSize) return std::nullopt;

    // 初始化槽位（仅第一次）
    if (!this->slotInitialized) {
        int64_t firstUs = this->audioFifo.front().pts().timestamp({ 1, 1000000 });
        // 计算起始槽位：四舍五入到最近的帧边界
        this->slotCounter = (firstUs * (int64_t)this->aencoder.sampleRate() + 500000) / 1000000 / this->frameSize;
        this->slotInitialized = true;
    }

    // 从 audioFifo 凑满一帧（不关心样本自带 PTS，因为我们将使用槽位 PTS）
    av::AudioSamples frame;
    if (frame.init(this->aencoder.sampleFormat(), this->frameSize,
        this->aencoder.channelLayout(), this->aencoder.sampleRate()) < 0) {
        this->LogError("分配音频帧失败");
        this->audioFifo.clear();
        return std::nullopt;
    }

    int copied = 0;
    while (copied < this->frameSize && !this->audioFifo.empty()) {
        auto& front = this->audioFifo.front();
        int need = this->frameSize - copied;
        int available = front.samplesCount();
        int take = std::min(need, available);

        CopyAudioSamples(frame, copied, front, 0, take);
        copied += take;

        if (take == available) {
            this->audioFifo.pop_front();
        }
        else {
            int remain = available - take;
            av::AudioSamples remainder;
            if (remainder.init(front.sampleFormat(), remain, front.channelsLayout(), front.sampleRate()) < 0) {
                this->LogError("分配剩余缓冲失败"); this->audioFifo.pop_front(); break;
            }
            CopyAudioSamples(remainder, 0, front, take, remain);
            remainder.setTimeBase(front.timeBase());
            if (front.pts()) {
                remainder.setPts(front.pts() + av::Timestamp(take, { 1, front.sampleRate() }));
            }
            this->audioFifo.front() = std::move(remainder);
            break;
        }
    }

    if (copied != this->frameSize) {
        this->LogError("音频样本不足，无法组装完整编码帧");
        return std::nullopt;
    }

    int64_t ptsSamples = this->slotCounter * this->frameSize;
    this->slotCounter++;

    frame.setTimeBase({ 1, this->aencoder.sampleRate() });
    frame.setPts(av::Timestamp(ptsSamples, { 1, this->aencoder.sampleRate() }));

    try {
        av::Packet pkt = this->aencoder.encode(frame);
        if (pkt && pkt.size() > 0) {
            this->SetPacketTiming(pkt, ptsSamples);
            this->lastStreamDuration = pkt.pts().seconds();
            return pkt;
        }
    }
    catch (const std::exception& e) { this->LogError(std::format("音频编码失败: {}", e.what())); }
    return std::nullopt;
}

std::optional<av::Packet> AEncodeHelper::Encode() {
    if (!this->aencoder.isOpened() || this->frameSize <= 0) return std::nullopt;
    int64_t totalSamples = 0;
    for (const auto& buf : this->audioFifo) totalSamples += buf.samplesCount();
    while (totalSamples < this->frameSize) {
        av::AudioSamples asamples;
        if (!this->pendingAudioSamples.try_dequeue(asamples)) break;
        if (asamples.samplesCount() <= 0) continue;
        av::AudioSamples converted;
        if (!this->CheckResampler(converted, std::move(asamples))) continue;
        this->audioFifo.push_back(std::move(converted));
        totalSamples += this->audioFifo.back().samplesCount();
    }
    return this->EncodeOneFrame();
}

void AEncodeHelper::FlushAll() {
    if (!this->aencoder.isOpened()) return;
    this->LogInfo("[冲刷] 开始最终冲刷流程");
    if (this->frameSize <= 0) {
        this->LogError("编码器帧长无效，无法冲刷音频");
        return;
    }

    // 确保槽位已初始化（如果此前从未编码过）
    if (!this->slotInitialized && !this->audioFifo.empty()) {
        int64_t firstUs = this->audioFifo.front().pts().timestamp({ 1, 1000000 });
        this->slotCounter = (firstUs * (int64_t)this->aencoder.sampleRate() + 500000) / 1000000 / this->frameSize;
        this->slotInitialized = true;
    }

    int flushFrameCount = 0;
    while (true) {
        auto pkt = this->Encode();
        if (!pkt.has_value()) break;
        this->flushPackets.push_back(std::move(*pkt));
        ++flushFrameCount;
    }
    this->LogInfo(std::format("[冲刷] 队列排空后共额外编码 {} 帧", flushFrameCount));

    // 处理残留样本（填充一帧）
    if (!this->audioFifo.empty() && this->frameSize > 0) {
        int residueSamples = 0;
        for (const auto& buf : this->audioFifo) residueSamples += buf.samplesCount();
        this->LogInfo(std::format("[冲刷] 处理残留样本, 共 {} 个样本", residueSamples));
        av::AudioSamples paddedFrame;
        if (paddedFrame.init(this->aencoder.sampleFormat(), this->frameSize,
            this->aencoder.channelLayout(), this->aencoder.sampleRate()) < 0) {
            this->LogError("无法分配填充帧"); this->audioFifo.clear();
        }
        else {
            int planes = paddedFrame.sampleFormat().isPlanar() ? paddedFrame.channelsCount() : 1;
            for (int i = 0; i < planes; ++i) std::memset(paddedFrame.data(i), 0, paddedFrame.size(i));
            int offset = 0;
            while (!this->audioFifo.empty() && offset < residueSamples) {
                auto& front = this->audioFifo.front();
                int take = std::min(front.samplesCount(), residueSamples - offset);
                CopyAudioSamples(paddedFrame, offset, front, 0, take);
                offset += take;
                if (take == front.samplesCount()) this->audioFifo.pop_front();
                else break;
            }
            int64_t ptsSamples = this->slotCounter * this->frameSize;
            this->slotCounter++;
            paddedFrame.setTimeBase({ 1, this->aencoder.sampleRate() });
            paddedFrame.setPts(av::Timestamp(ptsSamples, { 1, this->aencoder.sampleRate() }));

            try {
                av::Packet pkt = this->aencoder.encode(paddedFrame);
                if (pkt && pkt.size() > 0) {
                    this->SetPacketTiming(pkt, ptsSamples);
                    this->lastStreamDuration = pkt.pts().seconds();
                    this->flushPackets.push_back(std::move(pkt));
                    this->LogInfo("[冲刷] 填充帧编码成功");
                }
            }
            catch (...) { this->LogError("填充帧编码失败"); }
        }
    }

    // 冲刷编码器内部缓冲，使用相同的槽位计数器继续递增
    this->LogInfo("[冲刷] 开始冲刷编码器内部缓冲");
    int internalPkts = 0;
    try {
        while (true) {
            av::Packet pkt = this->aencoder.encode();
            if (!pkt || pkt.size() <= 0) break;

            int64_t ptsSamples = this->slotCounter * this->frameSize;
            this->slotCounter++;
            this->SetPacketTiming(pkt, ptsSamples);
            this->flushPackets.push_back(std::move(pkt));
            ++internalPkts;
        }
    }
    catch (...) { this->LogError("冲刷编码器失败"); }
    this->LogInfo(std::format("[冲刷] 编码器内部冲刷出 {} 个包", internalPkts));

    if (this->aencoder.isValid()) {
        this->aencoder.close();
        this->aencoder = {};
    }
    this->astream = av::Stream();
    this->audioFifo.clear();
    this->LogInfo("[冲刷] 音频编码器已关闭");
}

std::optional<av::Packet> AEncodeHelper::TrySetOff() {
    if (!this->flushPackets.empty()) {
        av::Packet pkt = std::move(this->flushPackets.front());
        this->flushPackets.pop_front();
        return pkt;
    }
    if (!this->aencoder.isOpened())return std::nullopt;
    this->FlushAll();
    if (!this->flushPackets.empty()) {
        av::Packet pkt = std::move(this->flushPackets.front());
        this->flushPackets.pop_front();
        return pkt;
    }
    return std::nullopt;
}