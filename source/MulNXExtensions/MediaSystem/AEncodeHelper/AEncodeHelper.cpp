#include "AEncodeHelper.hpp"
#include <MulNXExtensions/MediaSystem/AudioCapturer/AudioCapturer.hpp>

bool AEncodeHelper::Init() {
    this->pAudioCapturer = this->FindModule<AudioCapturer>("AudioCapturer");
    this->SubscribeSync("MediaSync/Reset", [this](auto&&...) { this->Reset(); });
    this->SubscribeSync("MediaSync/SetOn", [this](MulNX::Message& msg) {
        auto&& [info] = msg.Access<MulNX::AVStartInfo>();
        this->SetOn(info);
        });
    this->SubscribeSync("MediaSync/StateReport", [this](auto&&...) {
        this->LogInfo(std::format("音频编码器状态: {}", (this->aencoder.isOpened() ? "已打开" : "未打开")));
        this->LogInfo(std::format("音频流状态: {}", (this->astream.isValid() ? "有效" : "无效")));
        this->LogInfo(std::format("音频流总时长: {}秒", this->lastStreamDuration.load()));
        });
    return true;
}

void AEncodeHelper::SetOn(const MulNX::AVStartInfo& info) {
    if (this->pMediaState->advancedMode) { this->LogInfo("高级录制模式，不打开音频捕获"); return; }
    auto* oCtx = info.pOutCtx;
    this->audioFifo.clear();
    this->flushPackets.clear();
    int sampleRate = this->pAudioCapturer->GetSampleRate();
    if (sampleRate <= 0) { this->LogWarning("音频采样率无效"); return; }

    av::Codec acodec = av::findEncodingCodec(AV_CODEC_ID_AAC);
    if (!acodec.canEncode()) { this->LogWarning("AAC 编码器不可用，仅录制视频"); return; }
    auto supportedFmts = acodec.supportedSampleFormats();
    if (supportedFmts.empty()) { this->LogWarning("AAC 编码器未报告支持的样本格式，跳过音频"); return; }
    av::SampleFormat targetFmt = supportedFmts.front();
    this->LogWarning(std::format("音频编码器目标格式: {}", targetFmt.name()));

    this->aencoder = av::AudioEncoderContext(acodec);
    this->aencoder.setSampleRate(sampleRate);
    this->aencoder.setChannels(2);
    this->aencoder.setSampleFormat(targetFmt);
    this->aencoder.setChannelLayout(AV_CH_LAYOUT_STEREO);
    this->aencoder.setTimeBase({ 1, sampleRate });
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

    this->astream = oCtx->addStream(this->aencoder);
    this->astream.setTimeBase({ 1, sampleRate });
    av::CodecParameters cp;
    cp.copyFrom(this->aencoder);
    this->astream.setCodecParameters(cp);

    // 重置槽位计数器
    m_slotCounter = 0;
    m_slotInitialized = false;

    this->LogInfo(std::format("音频开启=m{}", (this->aencoder.isOpened() ? "是" : "否")));
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
            this->aresampler.srcChannels() != asamples.channelsCount()) {
            std::error_code err;
            bool ok = this->aresampler.init(
                this->aencoder.channelLayout(), this->aencoder.sampleRate(), this->aencoder.sampleFormat(),
                asamples.channelsLayout(), asamples.sampleRate(), asamples.sampleFormat(), err);
            if (!ok) { this->LogError(std::string("AudioResampler 初始化失败: ") + (err ? err.message() : "unknown")); return false; }
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

std::optional<av::Packet> AEncodeHelper::encodeOneFrame() {
    int totalSamples = 0;
    for (const auto& buf : this->audioFifo) totalSamples += buf.samplesCount();
    if (totalSamples < this->frameSize) return std::nullopt;

    // 初始化槽位（仅第一次）
    if (!m_slotInitialized) {
        int64_t firstUs = this->audioFifo.front().pts().timestamp({ 1, 1000000 });
        // 计算起始槽位：四舍五入到最近的帧边界
        m_slotCounter = (firstUs * (int64_t)this->aencoder.sampleRate() + 500000) / 1000000 / this->frameSize;
        m_slotInitialized = true;
    }

    // 从 audioFifo 凑满一帧（不关心样本自带 PTS，因为我们将使用槽位 PTS）
    av::AudioSamples frame;
    if (frame.init(this->aencoder.sampleFormat(), this->frameSize,
        this->aencoder.channelLayout(), this->aencoder.sampleRate()) < 0) {
        this->LogError("分配音频帧失败");
        this->audioFifo.clear();
        return std::nullopt;
    }

    int copied = 0, offsetInFrame = 0;
    while (copied < this->frameSize && !this->audioFifo.empty()) {
        auto& front = this->audioFifo.front();
        int need = this->frameSize - copied;
        int available = front.samplesCount();
        int take = std::min(need, available);

        if (!front.isPlanar()) {
            int bps = front.sampleFormat().bytesPerSample(), ch = front.channelsCount();
            memcpy(reinterpret_cast<uint8_t*>(frame.data(0)) + offsetInFrame * ch * bps,
                front.data(0), take * ch * bps);
        }
        else {
            int bps = front.sampleFormat().bytesPerSample();
            for (int c = 0; c < front.channelsCount(); ++c)
                memcpy(reinterpret_cast<uint8_t*>(frame.data(c)) + offsetInFrame * bps,
                    front.data(c), take * bps);
        }
        offsetInFrame += take; copied += take;

        if (take == available) {
            this->audioFifo.pop_front();
        }
        else {
            int remain = available - take;
            av::AudioSamples remainder;
            if (remainder.init(front.sampleFormat(), remain, front.channelsLayout(), front.sampleRate()) < 0) {
                this->LogError("分配剩余缓冲失败"); this->audioFifo.pop_front(); break;
            }
            if (!front.isPlanar()) {
                int bps = front.sampleFormat().bytesPerSample(), ch = front.channelsCount();
                memcpy(remainder.data(0), reinterpret_cast<const uint8_t*>(front.data(0)) + take * ch * bps, remain * ch * bps);
            }
            else {
                int bps = front.sampleFormat().bytesPerSample();
                for (int c = 0; c < front.channelsCount(); ++c)
                    memcpy(remainder.data(c), reinterpret_cast<const uint8_t*>(front.data(c)) + take * bps, remain * bps);
            }
            // 不关心剩余部分的时间戳，因为我们不再使用它
            remainder.setTimeBase({ 1, 1000000 });
            remainder.setPts(av::Timestamp(0, { 1, 1000000 }));
            this->audioFifo.front() = std::move(remainder);
            break;
        }
    }

    int64_t ptsSamples = m_slotCounter * this->frameSize;
    m_slotCounter++;

    frame.setTimeBase({ 1, this->aencoder.sampleRate() });
    frame.setPts(av::Timestamp(ptsSamples, { 1, this->aencoder.sampleRate() }));

    try {
        av::Packet pkt = this->aencoder.encode(frame);
        if (pkt && pkt.size() > 0) {
            pkt.setStreamIndex(this->astream.index());
            pkt.setTimeBase(this->astream.timeBase());
            pkt.setPts(frame.pts());
            pkt.setDuration(this->frameSize, this->astream.timeBase());
            pkt.raw()->duration = this->frameSize;     // 终极保障
            this->lastStreamDuration = pkt.pts().seconds();
            return pkt;
        }
    }
    catch (const std::exception& e) { this->LogError(std::string("音频编码失败: ") + e.what()); }
    return std::nullopt;
}

std::optional<av::Packet> AEncodeHelper::Encode() {
    if (!this->aencoder.isOpened() || this->frameSize <= 0) return std::nullopt;
    int totalSamples = 0;
    for (const auto& buf : this->audioFifo) totalSamples += buf.samplesCount();
    while (totalSamples < this->frameSize) {
        av::AudioSamples asamples;
        if (!this->bufferAudioSampleses.try_dequeue(asamples)) break;
        if (asamples.samplesCount() <= 0) continue;
        av::AudioSamples converted;
        if (!this->CheckResampler(converted, std::move(asamples))) continue;
        this->audioFifo.push_back(std::move(converted));
        totalSamples += this->audioFifo.back().samplesCount();
    }
    return this->encodeOneFrame();
}

void AEncodeHelper::flushAll() {
    if (!this->aencoder.isOpened()) return;
    this->LogInfo("[冲刷] 开始最终冲刷流程");

    // 确保槽位已初始化（如果此前从未编码过）
    if (!m_slotInitialized && !this->audioFifo.empty()) {
        int64_t firstUs = this->audioFifo.front().pts().timestamp({ 1, 1000000 });
        m_slotCounter = (firstUs * (int64_t)this->aencoder.sampleRate() + 500000) / 1000000 / this->frameSize;
        m_slotInitialized = true;
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
                if (!front.isPlanar()) {
                    int bps = front.sampleFormat().bytesPerSample(), ch = front.channelsCount();
                    memcpy(reinterpret_cast<uint8_t*>(paddedFrame.data(0)) + offset * ch * bps, front.data(0), take * ch * bps);
                }
                else {
                    int bps = front.sampleFormat().bytesPerSample();
                    for (int c = 0; c < front.channelsCount(); ++c)
                        memcpy(reinterpret_cast<uint8_t*>(paddedFrame.data(c)) + offset * bps, front.data(c), take * bps);
                }
                offset += take;
                if (take == front.samplesCount()) this->audioFifo.pop_front();
                else break;
            }
            int64_t ptsSamples = m_slotCounter * this->frameSize;
            m_slotCounter++;
            paddedFrame.setTimeBase({ 1, this->aencoder.sampleRate() });
            paddedFrame.setPts(av::Timestamp(ptsSamples, { 1, this->aencoder.sampleRate() }));

            try {
                av::Packet pkt = this->aencoder.encode(paddedFrame);
                if (pkt && pkt.size() > 0) {
                    pkt.setStreamIndex(this->astream.index());
                    pkt.setTimeBase(this->astream.timeBase());
                    pkt.setPts(paddedFrame.pts());
                    pkt.setDuration(this->frameSize, this->astream.timeBase());
                    pkt.raw()->duration = this->frameSize;
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

            int64_t ptsSamples = m_slotCounter * this->frameSize;
            m_slotCounter++;
            pkt.setStreamIndex(this->astream.index());
            pkt.setTimeBase(this->astream.timeBase());
            pkt.setPts(ptsSamples, this->astream.timeBase());
            pkt.setDuration(this->frameSize, this->astream.timeBase());
            pkt.raw()->duration = this->frameSize;
            this->flushPackets.push_back(std::move(pkt));
            ++internalPkts;
        }
    }
    catch (...) { this->LogError("冲刷编码器失败"); }
    this->LogInfo(std::format("[冲刷] 编码器内部冲刷出 {} 个包，PTS 已完美对齐", internalPkts));

    // 最终遍历，确保 duration 万无一失
    int corrected = 0;
    for (auto& pkt : this->flushPackets) {
        if (pkt.raw()->duration != this->frameSize) {
            pkt.raw()->duration = this->frameSize;
            pkt.setDuration(this->frameSize, this->astream.timeBase());
            ++corrected;
        }
    }
    if (corrected > 0) {
        this->LogInfo(std::format("[冲刷] 最终再修正 {} 个包的 duration", corrected));
    }

    if (this->aencoder.isValid()) { this->aencoder.close(); this->aencoder = {}; }
    this->astream = av::Stream();
    this->audioFifo.clear();
    this->LogInfo("[冲刷] 音频编码器已关闭");
}

std::optional<av::Packet> AEncodeHelper::TrySetOff() {
    if (!this->aencoder.isOpened() && this->flushPackets.empty()) return std::nullopt;
    if (this->aencoder.isOpened()) this->flushAll();
    if (!this->flushPackets.empty()) {
        av::Packet pkt = std::move(this->flushPackets.front());
        this->flushPackets.pop_front();
        return pkt;
    }
    return std::nullopt;
}

void AEncodeHelper::Reset() {
    av::AudioSamples temp;
    while (this->bufferAudioSampleses.try_dequeue(temp)) {}
    this->audioFifo.clear();
    this->flushPackets.clear();
    if (this->aencoder.isValid() && this->aencoder.isOpened()) this->aencoder.close();
    this->aencoder = {};
    this->astream = av::Stream();
    this->aresampler = {};
    this->frameSize = 0;
    this->lastStreamDuration = 0.0;
    m_slotInitialized = false;
    m_slotCounter = 0;
}