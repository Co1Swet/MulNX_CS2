#include "AEncodeHelper.hpp"
#include <MulNXExtensions/MediaSystem/AudioCapturer/AudioCapturer.hpp>

// 辅助函数：微秒 -> 采样点数（四舍五入）
static inline int64_t UsToSamples(int64_t us, int sampleRate) {
    return (us * sampleRate + 500'000) / 1'000'000;
}

bool AEncodeHelper::Init() {
    this->pAudioCapturer = this->FindModule<AudioCapturer>("AudioCapturer");

    this->SubscribeSync("MediaSync/Reset", [this](auto&&...) {
        this->Reset();
        });

    this->SubscribeSync("MediaSync/SetOn", [this](MulNX::Message& msg) {
        auto&& [info] = msg.Access<MulNX::AVStartInfo>();
        this->SetOn(info);
        });

    return true;
}

void AEncodeHelper::SetOn(const MulNX::AVStartInfo& info) {
    if (this->pMediaState->advancedMode) {
        this->LogInfo("高级录制模式，不打开音频捕获");
        return;
    }

    auto* oCtx = info.pOutCtx;

    this->audioFifo.clear();
    int sampleRate = this->pAudioCapturer->GetSampleRate();

    av::Codec acodec = av::findEncodingCodec(AV_CODEC_ID_AAC);
    if (!acodec.canEncode()) {
        this->LogWarning("AAC 编码器不可用，仅录制视频");
        return;
    }
    auto supportedFmts = acodec.supportedSampleFormats();
    if (supportedFmts.empty()) {
        this->LogWarning("AAC 编码器未报告支持的样本格式，跳过音频");
        return;
    }
    av::SampleFormat targetFmt = supportedFmts.front();
    this->LogWarning(std::string("音频编码器目标格式: ") + targetFmt.name());

    this->aencoder = av::AudioEncoderContext(acodec);
    int outChannels = 2;
    uint64_t outLayout = AV_CH_LAYOUT_STEREO;

    this->aencoder.setSampleRate(sampleRate);
    this->aencoder.setChannels(outChannels);
    this->aencoder.setSampleFormat(targetFmt);
    this->aencoder.setChannelLayout(outLayout);
    this->aencoder.setTimeBase({ 1, sampleRate });
    this->aencoder.setBitRate(128000);

    try {
        this->aencoder.open();
        this->LogWarning(std::string("音频编码器已打开，frame_size=") + std::to_string(this->aencoder.frameSize()));
    }
    catch (const std::exception& e) {
        this->LogError(std::string("音频编码器打开失败: ") + e.what());
        this->aencoder = {};
        return;
    }

    this->astream = oCtx->addStream(this->aencoder);
    this->astream.setTimeBase({ 1, sampleRate });

    av::CodecParameters cp;
    cp.copyFrom(this->aencoder);
    this->astream.setCodecParameters(cp);

    try {
        this->LogInfo(std::format("音频开启=m{}", (this->aencoder.isOpened() ? "是" : "否")));
    }
    catch (const std::exception& e) {
        this->LogError(std::format("输出提示失败，错误信息：{}", e.what()));
    }
}

std::optional<av::Packet> AEncodeHelper::TrySetOff() {
    if (!this->aencoder.isOpened()) return std::nullopt;

    int frameSize = this->aencoder.frameSize();
    if (frameSize <= 0) return std::nullopt;
    
    auto h = this->Encode();
    if (h.has_value())return h;

    // 不足一帧，冲洗编码器内部缓冲
    av::Packet pkt = this->aencoder.encode();
    if (pkt && pkt.size() > 0) {
        pkt.setStreamIndex(this->astream.index());
        pkt.setTimeBase(this->astream.timeBase());
        return pkt;
    }

    // 全部结束，关闭编码器
    if (this->aencoder.isValid())
        this->aencoder.close();
    this->astream = av::Stream();
    this->audioFifo.clear();

    return std::nullopt;
}

void AEncodeHelper::Reset() {
    av::AudioSamples temp;
    while (this->bufferAudioSampleses.try_dequeue(temp)) {
        // 清空队列
    }
}

bool AEncodeHelper::CheckResampler(av::AudioSamples& converted, av::AudioSamples&& asamples) {
    // 1. 保存原始时间戳（微秒）
    int64_t originalUs = asamples.pts().timestamp({ 1, 1000000 });

    // 2. 判断是否需要重采样
    bool needResample = (asamples.sampleFormat() != this->aencoder.sampleFormat()) ||
        (asamples.sampleRate() != this->aencoder.sampleRate()) ||
        (asamples.channelsLayout() != this->aencoder.channelLayout());

    if (needResample) {
        // 初始化或更新重采样器
        if (!this->aresampler.isValid() ||
            this->aresampler.srcSampleRate() != asamples.sampleRate() ||
            this->aresampler.srcSampleFormat() != asamples.sampleFormat() ||
            this->aresampler.srcChannels() != asamples.channelsCount()) {
            std::error_code err;
            bool ok = this->aresampler.init(
                this->aencoder.channelLayout(), this->aencoder.sampleRate(), this->aencoder.sampleFormat(),
                asamples.channelsLayout(), asamples.sampleRate(), asamples.sampleFormat(), err);
            if (!ok) {
                this->LogError(std::string("AudioResampler 初始化失败: ") + (err ? err.message() : "unknown"));
                return false;
            }
        }

        this->aresampler.push(asamples);
        converted = this->aresampler.pop(0);
        if (!converted.isValid() || converted.samplesCount() == 0) {
            return false;
        }
    }
    else {
        converted = std::move(asamples);
    }

    // 3. 将原始 PTS 赋给转换后的样本
    converted.setTimeBase({ 1, 1000000 });
    converted.setPts(av::Timestamp(originalUs, { 1, 1000000 }));
    return true;
}

std::optional<av::Packet> AEncodeHelper::Encode() {
    if (!this->aencoder.isOpened()) return std::nullopt;

    av::AudioSamples asamples;
    if (!this->bufferAudioSampleses.try_dequeue(asamples)) return std::nullopt;
    if (asamples.samplesCount() <= 0) return std::nullopt;

    av::AudioSamples converted;
    if (!this->CheckResampler(converted, std::move(asamples))) return std::nullopt;

    // 推入 FIFO（已携带微秒 PTS）
    this->audioFifo.push_back(std::move(converted));

    int frameSize = this->aencoder.frameSize();
    if (frameSize <= 0) return std::nullopt;

    // 统计总样本数
    int totalSamples = 0;
    for (const auto& buf : this->audioFifo) totalSamples += buf.samplesCount();

    // 当累积不足一帧时，返回空
    if (totalSamples < frameSize)return std::nullopt;

    av::AudioSamples frame;
    if (frame.init(this->aencoder.sampleFormat(), frameSize,
        this->aencoder.channelLayout(), this->aencoder.sampleRate()) < 0) {
        this->LogError("分配音频帧失败");
        return std::nullopt;
    }

    // 获取帧起始 PTS
    int64_t frameStartUs = -1;
    if (!this->audioFifo.empty()) {
        frameStartUs = this->audioFifo.front().pts().timestamp({ 1, 1000000 });
    }

    int copied = 0;
    int offsetInFrame = 0;
    while (copied < frameSize && !this->audioFifo.empty()) {
        auto& front = this->audioFifo.front();
        int need = frameSize - copied;
        int available = front.samplesCount();
        int take = std::min(need, available);

        // 拷贝样本数据
        if (!front.isPlanar()) {
            int bps = front.sampleFormat().bytesPerSample();
            int ch = front.channelsCount();
            memcpy(reinterpret_cast<uint8_t*>(frame.data(0)) + offsetInFrame * ch * bps,
                front.data(0), take * ch * bps);
        }
        else {
            int bps = front.sampleFormat().bytesPerSample();
            for (int c = 0; c < front.channelsCount(); ++c) {
                memcpy(reinterpret_cast<uint8_t*>(frame.data(c)) + offsetInFrame * bps,
                    front.data(c), take * bps);
            }
        }

        offsetInFrame += take;
        copied += take;

        if (take == available) {
            this->audioFifo.pop_front();
        }
        else {
            // 部分消费：保留剩余部分并修正其 PTS
            int remain = available - take;
            av::AudioSamples remainder;
            if (remainder.init(front.sampleFormat(), remain, front.channelsLayout(), front.sampleRate()) >= 0) {
                if (!front.isPlanar()) {
                    int bps = front.sampleFormat().bytesPerSample();
                    int ch = front.channelsCount();
                    memcpy(remainder.data(0),
                        reinterpret_cast<const uint8_t*>(front.data(0)) + take * ch * bps,
                        remain * ch * bps);
                }
                else {
                    int bps = front.sampleFormat().bytesPerSample();
                    for (int c = 0; c < front.channelsCount(); ++c) {
                        memcpy(remainder.data(c),
                            reinterpret_cast<const uint8_t*>(front.data(c)) + take * bps,
                            remain * bps);
                    }
                }
                // 计算新 PTS
                int64_t originalUs = front.pts().timestamp({ 1, 1000000 });
                int64_t takeUs = (int64_t)take * 1000000 / this->aencoder.sampleRate();
                remainder.setTimeBase({ 1, 1000000 });
                remainder.setPts(av::Timestamp(originalUs + takeUs, { 1, 1000000 }));
                this->audioFifo.front() = std::move(remainder);
            }
            else {
                this->audioFifo.pop_front();
            }
            break; // 已取够
        }
    }

    // 设置帧的 PTS（转换为编码器时间基）
    if (frameStartUs >= 0) {
        int64_t ptsSamples = UsToSamples(frameStartUs, this->aencoder.sampleRate());
        frame.setTimeBase({ 1, this->aencoder.sampleRate() });
        frame.setPts(av::Timestamp(ptsSamples, { 1, this->aencoder.sampleRate() }));
    }
    else {
        frame.setTimeBase({ 1, this->aencoder.sampleRate() });
        frame.setPts(av::Timestamp(0, { 1, this->aencoder.sampleRate() }));
    }

    try {
        av::Packet pkt = this->aencoder.encode(frame);
        if (pkt && pkt.size() > 0) {
            pkt.setStreamIndex(this->astream.index());
            pkt.setTimeBase(this->astream.timeBase());
            pkt.setDuration(frameSize, this->astream.timeBase());
            return pkt;
        }
    }
    catch (const std::exception& e) {
        this->LogError(std::string("音频编码失败: ") + e.what());
    }

    return std::nullopt;
}