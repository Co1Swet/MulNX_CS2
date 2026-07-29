#include "AEncodeHelper.hpp"

bool AEncodeHelper::Init() {
    this->SubscribeSync("MediaSync/Reset", [this](auto&&...) {
        this->Reset();
        });
    
    return true;
}

void AEncodeHelper::SetOn(av::FormatContext* oCtx, int sampleRate) {
    this->audioFifo.clear(); // 清空音频缓冲

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
    // 取第一个支持的格式（通常是 fltp）
    av::SampleFormat targetFmt = supportedFmts.front();
    this->LogWarning(std::string("音频编码器目标格式: ") + targetFmt.name());

    this->aencoder = av::AudioEncoderContext(acodec);
    int outChannels = 2;
    uint64_t outLayout = AV_CH_LAYOUT_STEREO;
    this->aptsCounter = 0;

    this->aencoder.setSampleRate(sampleRate);
    this->aencoder.setChannels(outChannels);
    this->aencoder.setSampleFormat(targetFmt);
    this->aencoder.setChannelLayout(outLayout);
    this->aencoder.setBitRate(128000);

    try {
        this->aencoder.open();
        this->LogWarning(std::string("音频编码器已打开，frame_size=") + std::to_string(this->aencoder.frameSize()));
    }
    catch (const std::exception& e) {
        this->LogError(std::string("音频编码器打开失败: ") + e.what());
        this->aencoder = {}; // 重置
        return;
    }

    this->astream = oCtx->addStream(this->aencoder);
    this->astream.setTimeBase({ 1, sampleRate });

    av::CodecParameters cp;
    cp.copyFrom(this->aencoder);
    this->astream.setCodecParameters(cp);

    try {
        this->LogInfo(std::format(" 音频开启=m{}", (this->aencoder.isOpened() ? "是" : "否")));
    }
    catch (const std::exception& e) {
        this->LogError(std::format("输出提示失败，错误信息：{}", e.what()));
    }
}

std::optional<av::Packet> AEncodeHelper::TrySetOff() {
    if (!this->aencoder.isOpened()) return std::nullopt;

    int frameSize = this->aencoder.frameSize();
    if (frameSize > 0 && !this->audioFifo.empty()) {
        av::AudioSamples frame;
        if (frame.init(this->aencoder.sampleFormat(), frameSize, this->aencoder.channelLayout(), this->aencoder.sampleRate()) >= 0) {
            // 默认初始化后的样本为静音
            int copied = 0;
            while (copied < frameSize && !this->audioFifo.empty()) {
                auto &front = this->audioFifo.front();
                int need = frameSize - copied;
                int available = front.samplesCount();
                int take = std::min(need, available);

                if (!front.isPlanar()) {
                    int bps = front.sampleFormat().bytesPerSample();
                    int ch = front.channelsCount();
                    memcpy(reinterpret_cast<uint8_t*>(frame.data(0)) + static_cast<size_t>(copied) * ch * bps,
                        front.data(0), static_cast<size_t>(take) * ch * bps);
                }
                else {
                    int bps = front.sampleFormat().bytesPerSample();
                    for (int c = 0; c < front.channelsCount(); ++c) {
                        memcpy(reinterpret_cast<uint8_t*>(frame.data(c)) + static_cast<size_t>(copied) * bps,
                            front.data(c), static_cast<size_t>(take) * bps);
                    }
                }

                copied += take;
                if (take == available) {
                    this->audioFifo.pop_front();
                }
                else {
                    int remain = available - take;
                    av::AudioSamples remaining;
                    if (remaining.init(front.sampleFormat(), remain, front.channelsLayout(), front.sampleRate()) >= 0) {
                        if (!front.isPlanar()) {
                            int bps = front.sampleFormat().bytesPerSample();
                            int ch = front.channelsCount();
                            memcpy(remaining.data(0),
                                reinterpret_cast<const uint8_t*>(front.data(0)) + static_cast<size_t>(take) * ch * bps,
                                static_cast<size_t>(remain) * ch * bps);
                        }
                        else {
                            int bps = front.sampleFormat().bytesPerSample();
                            for (int c = 0; c < front.channelsCount(); ++c) {
                                memcpy(remaining.data(c),
                                    reinterpret_cast<const uint8_t*>(front.data(c)) + static_cast<size_t>(take) * bps,
                                    static_cast<size_t>(remain) * bps);
                            }
                        }
                        this->audioFifo.front() = std::move(remaining);
                    }
                    else {
                        this->audioFifo.pop_front();
                    }
                }
            }

            frame.setTimeBase({ 1, this->aencoder.sampleRate() });
            frame.setPts(av::Timestamp(this->aptsCounter, frame.timeBase()));
            this->aptsCounter += frameSize;
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
                this->LogError(std::string("音频刷新编码失败: ") + e.what());
            }
        }
        this->audioFifo.clear();
    }

    av::Packet apkt = this->aencoder.encode();
    if (apkt && apkt.size() != 0) {
        apkt.setStreamIndex(this->astream.index());
        apkt.setTimeBase(this->astream.timeBase());
        return apkt;
    }
    
    if (this->aencoder.isValid()) {
        this->aencoder.close();
    }
    this->astream = av::Stream();
    this->audioFifo.clear();

    return std::nullopt;
}

void AEncodeHelper::Reset() {
    this->aptsCounter = 0;
}

bool AEncodeHelper::CheckResampler(av::AudioSamples& converted, av::AudioSamples&& asamples) {
    // 1. 格式转换与重采样（如果需要）
    bool needResample = (asamples.sampleFormat() != this->aencoder.sampleFormat()) ||
        (asamples.sampleRate() != this->aencoder.sampleRate()) ||
        (asamples.channelsLayout() != this->aencoder.channelLayout());

    if (needResample) {
        // 检查或初始化 resampler
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
            this->LogWarning("AudioResampler 已初始化");
        }

        this->aresampler.push(asamples);
        converted = this->aresampler.pop(0);
        if (!converted.isValid() || converted.samplesCount() == 0) {
            // 重采样器需要更多数据才能输出，正常等待
            return false;
        }
    }
    else {
        // 格式已经匹配，直接使用
        converted = std::move(asamples);
    }
    return true;
}

std::optional<av::Packet> AEncodeHelper::Encode(av::AudioSamples&& asamples) {
    if (!this->aencoder.isOpened())return std::nullopt;

    av::AudioSamples converted;
    if (!this->CheckResampler(converted, std::move(asamples)))return std::nullopt;

    // 2. 按编码器帧大小送入编码器
    int frameSize = this->aencoder.frameSize();
    if (frameSize <= 0) {
        // 无固定帧大小，直接编码
        converted.setTimeBase({ 1, this->aencoder.sampleRate() });
        converted.setPts(av::Timestamp(this->aptsCounter, converted.timeBase()));
        av::Packet pkt = this->aencoder.encode(converted);
        this->aptsCounter += converted.samplesCount();
        if (pkt && pkt.size() > 0) {
            pkt.setStreamIndex(this->astream.index());
            pkt.setTimeBase(this->astream.timeBase());
            pkt.setDuration(converted.samplesCount(), this->astream.timeBase());
            return pkt;
        }
        return std::nullopt;
    }

    // ---- 帧大小对齐处理：使用 audioFifo 累积采样 ----
    // 将 converted 追加到 audioFifo（deque<float> 或 deque<sample_type>）
    // 注意：这里假设编码器格式为 FLTP，所以我们存储为 float 平面数据稍复杂。为简化，我们直接存储 AudioSamples 对象在队列中。
    this->audioFifo.push_back(converted);

    // 统计队列中总样本数
    int totalSamples = 0;
    for (const auto& buf : this->audioFifo) {
        totalSamples += buf.samplesCount();
    }

    // 当累积样本数 >= frameSize 时，取出 frameSize 个样本编码
    while (totalSamples >= frameSize) {
        // 分配一个帧的 AudioSamples
        av::AudioSamples frame;
        if (frame.init(this->aencoder.sampleFormat(), frameSize,
            this->aencoder.channelLayout(), this->aencoder.sampleRate()) < 0) {
            this->LogError("无法分配音频帧");
            break;
        }

        int copied = 0;
        int offsetInFrame = 0;
        // 从队列前端逐个取出 buffer，拷贝数据到 frame
        while (copied < frameSize && !this->audioFifo.empty()) {
            auto& front = this->audioFifo.front();
            int need = frameSize - copied;
            int available = front.samplesCount();
            int take = std::min(need, available);

            // 拷贝 take 个样本
            if (!front.isPlanar()) {
                int bps = front.sampleFormat().bytesPerSample();
                int ch = front.channelsCount();
                // 将 front 中 take 个交错样本拷贝到 frame 的对应位置
                memcpy(reinterpret_cast<uint8_t*>(frame.data(0)) + offsetInFrame * ch * bps,
                    front.data(0),
                    take * ch * bps);
            }
            else {
                int bps = front.sampleFormat().bytesPerSample();
                for (int c = 0; c < front.channelsCount(); ++c) {
                    memcpy(reinterpret_cast<uint8_t*>(frame.data(c)) + offsetInFrame * bps,
                        front.data(c),
                        take * bps);
                }
            }

            offsetInFrame += take;
            copied += take;

            // 如果 front 还有剩余，则保留；否则弹出
            if (take == available) {
                this->audioFifo.pop_front();
            }
            else {
                // 部分使用，需要创建剩余部分的 AudioSamples 替代原来的 front
                int remain = available - take;
                av::AudioSamples newFront;
                if (newFront.init(front.sampleFormat(), remain, front.channelsLayout(), front.sampleRate()) >= 0) {
                    if (!front.isPlanar()) {
                        int bps = front.sampleFormat().bytesPerSample();
                        int ch = front.channelsCount();
                        memcpy(newFront.data(0),
                            reinterpret_cast<const uint8_t*>(front.data(0)) + take * ch * bps,
                            remain * ch * bps);
                    }
                    else {
                        int bps = front.sampleFormat().bytesPerSample();
                        for (int c = 0; c < front.channelsCount(); ++c) {
                            memcpy(newFront.data(c),
                                reinterpret_cast<const uint8_t*>(front.data(c)) + take * bps,
                                remain * bps);
                        }
                    }
                    this->audioFifo.front() = std::move(newFront);
                }
                else {
                    // 分配失败，丢弃该帧剩余数据
                    this->audioFifo.pop_front();
                }
                // 已取够样本，跳出内层循环
                break;
            }
        }

        // 设置 PTS 并编码
        frame.setTimeBase({ 1, this->aencoder.sampleRate() });
        frame.setPts(av::Timestamp(this->aptsCounter, frame.timeBase()));

        try {
            av::Packet pkt = this->aencoder.encode(frame);
            this->aptsCounter += frameSize;
            if (pkt && pkt.size() > 0) {
                pkt.setStreamIndex(this->astream.index());
                pkt.setTimeBase(this->astream.timeBase());
                pkt.setDuration(frameSize, this->astream.timeBase());
                return pkt;
            }
            else {
                this->LogWarning("编码器未产生音频包");
            }
        }
        catch (const std::exception& e) {
            this->LogError(std::string("音频编码失败: ") + e.what());
        }

        this->aptsCounter += frameSize;
        totalSamples -= frameSize;
    }
    return std::nullopt;
}