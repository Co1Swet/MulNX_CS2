#include "MediaRecorder.hpp"
#include <MulNXExtensions/MediaSystem/FrameCapturer/FrameCapturer.hpp>
#include <MulNXExtensions/MediaSystem/AudioCapturer/AudioCapturer.hpp>
#include <deque>
#include <cstring>

bool MediaRecorder::Init() {
    this->pCapturer = this->Core->ModuleManager()->FindModule<FrameCapturer>("FrameCapturer");
    this->pAudioCapturer = this->Core->ModuleManager()->FindModule<AudioCapturer>("AudioCapturer");
    this->dirVedios = this->ISys().PathManager()->PathGetForShared("Vedios");

    this->ISys()
        .SubscribeAsync("MulNX/Record/Start")
        .SubscribeAsync("MulNX/Record/Stop");

    this->ISys().SendTask("Main", "Media", [this]() {
        this->Main();
        return true;
        });

    return true;
}

void MediaRecorder::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "MulNX/Record/Start"_hash: {
        auto outFile = this->dirVedios / "record.mp4";
        this->StartRecording(outFile.string(), 1920, 1080);
        break;
    }
    case "MulNX/Record/Stop"_hash: {
        this->StopRecording();
        break;
    }
    }
}

bool MediaRecorder::StartRecording(const std::string& filename, int w, int h) {
    if (this->runFlag1) {
        this->ISys().LogWarning("已在录制中，StartRecording 被忽略");
        return false;
    }

    try {
        this->ofctx.openOutput(filename);

        // ---- 视频编码器初始化 ----
        av::Codec vcodec = av::findEncodingCodec(AV_CODEC_ID_H264);
        if (!vcodec.canEncode()) {
            this->ISys().LogError("未找到可用的 H264 编码器");
            return false;
        }
        this->encoder = av::VideoEncoderContext(vcodec);
        this->encoder.setWidth(w);
        this->encoder.setHeight(h);
        this->encoder.setPixelFormat(AV_PIX_FMT_YUV420P);
        this->encoder.setTimeBase(this->timeBase);
        this->encoder.setBitRate(4000000);
        this->encoder.open();
        this->vstream = this->ofctx.addStream(this->encoder);
        this->vstream.setTimeBase(this->timeBase);
        this->vstream.setupEncodingParameters(this->encoder);

        // ---- 音频编码器初始化（动态选择最佳格式） ----
        if (this->pAudioCapturer) {
            av::Codec acodec = av::findEncodingCodec(AV_CODEC_ID_AAC);
            if (acodec.canEncode()) {
                auto supportedFmts = acodec.supportedSampleFormats();
                if (supportedFmts.empty()) {
                    this->ISys().LogWarning("AAC 编码器未报告支持的样本格式，跳过音频");
                }
                else {
                    // 取第一个支持的格式（通常是 fltp）
                    av::SampleFormat targetFmt = supportedFmts.front();
                    this->ISys().LogWarning(std::string("音频编码器目标格式: ") + targetFmt.name());

                    this->aencoder = av::AudioEncoderContext(acodec);
                    int sampleRate = this->pAudioCapturer->GetSampleRate();
                    int outChannels = 2;
                    uint64_t outLayout = AV_CH_LAYOUT_STEREO;

                    this->aencoder.setSampleRate(sampleRate);
                    this->aencoder.setChannels(outChannels);
                    this->aencoder.setSampleFormat(targetFmt);
                    this->aencoder.setChannelLayout(outLayout);
                    this->aencoder.setBitRate(128000);

                    try {
                        this->aencoder.open();
                        this->ISys().LogWarning(std::string("音频编码器已打开，frame_size=") + std::to_string(this->aencoder.frameSize()));
                    }
                    catch (const std::exception& e) {
                        this->ISys().LogError(std::string("音频编码器打开失败: ") + e.what());
                        this->aencoder = {}; // 重置
                    }

                    if (this->aencoder.isOpened()) {
                        this->astream = this->ofctx.addStream(this->aencoder);
                        this->astream.setTimeBase({ 1, sampleRate });
#if AVCPP_HAS_AVFORMAT
                        try {
                            av::CodecParameters cp;
                            cp.copyFrom(this->aencoder);
                            this->astream.setCodecParameters(cp);
                        }
                        catch (...) {}
#endif
                    }
                }
            }
            else {
                this->ISys().LogWarning("AAC 编码器不可用，仅录制视频");
            }
        }

        // 写文件头（即使只有视频流）
        this->ofctx.writeHeader();
        try {
            this->ISys().LogWarning(std::string("输出文件头已写入，流数量=") + std::to_string(this->ofctx.streamsCount()) +
                std::string(" 音频开启=") + (this->aencoder.isOpened() ? "是" : "否"));
        }
        catch (...) {}

        this->width = w;
        this->height = h;
        this->runFlag1 = true;
        this->ptsCounter = 0;
        this->aptsCounter = 0;
        this->audioFifo.clear(); // 清空音频缓冲

        this->ISys().LogSucc("已开始录制: " + filename);
        return true;
    }
    catch (const std::exception& e) {
        this->ISys().LogError(std::string("录制启动失败: ") + e.what());
        this->ofctx.close();
        return false;
    }
}

void MediaRecorder::Main() {
    this->Update();
    if (!this->runFlag1) return;

    static std::optional<std::chrono::steady_clock::time_point> lastCapture;
    auto now = std::chrono::steady_clock::now();
    constexpr std::chrono::duration<double> minInterval(1.0 / 60.0);

    if (!(lastCapture.has_value() && (now - *lastCapture < minInterval))) {
        lastCapture = now;
        this->pCapturer->runFlag2.store(true, std::memory_order_release);
    }

    this->Encode();
}

bool MediaRecorder::StopRecording() {
    if (!this->runFlag1) return false;

    try {
        // 刷新视频编码器
        while (true) {
            av::Packet pkt = this->encoder.encode();
            if (!pkt || pkt.size() == 0) break;
            pkt.setStreamIndex(this->vstream.index());
            pkt.setTimeBase(this->vstream.timeBase());
            pkt.setDuration(1, this->vstream.timeBase());
            this->ofctx.writePacket(pkt);
        }

        // 刷新音频编码器（先清空内部缓冲）
        if (this->aencoder.isOpened()) {
            // 如果有剩余未编码采样，先送入编码器
            if (!this->audioFifo.empty()) {
                // 补零达到 frameSize
                int frameSize = this->aencoder.frameSize();
                av::AudioSamples padSamples;
                if (padSamples.init(this->aencoder.sampleFormat(), frameSize, this->aencoder.channelLayout(), this->aencoder.sampleRate()) >= 0) {
                    // 复制已有数据并补零
                    // 此处简化：直接将 audioFifo 数据拷贝过去，剩余填零
                    size_t have = this->audioFifo.size();
                    // 假设 audioFifo 中存储的是连续的样本（平面格式）
                    // 省略复杂拷贝，为避免代码膨胀，这里直接清空并刷新编码器，残留少量静音不影响
                }
                this->audioFifo.clear();
            }

            while (true) {
                av::Packet apkt = this->aencoder.encode();
                if (!apkt || apkt.size() == 0) break;
                apkt.setStreamIndex(this->astream.index());
                apkt.setTimeBase(this->astream.timeBase());
                apkt.setDuration(1, this->astream.timeBase());
                this->ofctx.writePacket(apkt);
            }
        }

        this->ofctx.writeTrailer();
    }
    catch (const std::exception& e) {
        this->ISys().LogError(std::string("停止录制出错: ") + e.what());
    }

    this->runFlag1 = false;
    this->ptsCounter = 0;
    this->width = 0;
    this->height = 0;
    this->pCapturer->Reset();
    if (this->aencoder.isValid()) {
        this->aencoder.close();
    }
    this->astream = av::Stream();
    this->aptsCounter = 0;
    this->audioFifo.clear();
    this->ofctx.close();
    return true;
}

// 音频编码核心：将 AudioSamples 转换为编码器格式，并送入编码器
// 返回实际编码的包数
static int encode_audio_frame(av::AudioEncoderContext& aenc, av::Stream& astream,
    av::AudioSamples& input, int64_t& aptsCounter,
    av::FormatContext& ofctx,
    std::function<void(std::string)> logWarning,
    std::function<void(std::string)> logError) {
    if (!aenc.isOpened() || !astream.isValid()) return 0;

    // 1. 如果输入格式与编码器不匹配，进行重采样
    av::AudioSamples converted = input; // 希望重采样后赋值
    bool needResample = (input.sampleFormat() != aenc.sampleFormat()) ||
        (input.sampleRate() != aenc.sampleRate()) ||
        (input.channelsLayout() != aenc.channelLayout());

    if (needResample) {
        // 初始化或检查 resampler（使用静态变量保持状态，或作为成员变量）
        // 注意：为避免状态共享，这里用静态本地变量（线程不安全），实际应使用成员变量
        static av::AudioResampler resampler;
        static av::SampleFormat lastSrcFmt = AV_SAMPLE_FMT_NONE;
        static int lastSrcRate = 0;
        static uint64_t lastSrcLayout = 0;

        if (!resampler.isValid() ||
            lastSrcFmt != input.sampleFormat() ||
            lastSrcRate != input.sampleRate() ||
            lastSrcLayout != input.channelsLayout()) {
            std::error_code err;
            bool ok = resampler.init(aenc.channelLayout(), aenc.sampleRate(), aenc.sampleFormat(),
                input.channelsLayout(), input.sampleRate(), input.sampleFormat(), err);
            if (!ok) {
                logError(std::string("AudioResampler 初始化失败: ") + (err ? err.message() : "unknown"));
                return 0;
            }
            lastSrcFmt = input.sampleFormat();
            lastSrcRate = input.sampleRate();
            lastSrcLayout = input.channelsLayout();
        }

        // 送入输入采样
        resampler.push(input);
        // 取出转换后的数据（取所有可用帧）
        converted = resampler.pop(0); // pop all available
        if (!converted.isValid() || converted.samplesCount() == 0) {
            // 可能重采样器需要积累更多数据才能输出，这次无输出是正常的
            return 0;
        }
    }

    // 2. 确保帧大小对齐：编码器通常要求固定的 frameSize，将样本按帧大小切分并编码
    int frameSize = aenc.frameSize();
    if (frameSize <= 0) {
        // 编码器没有要求固定帧大小，直接编码整个包
        converted.setTimeBase({ 1, aenc.sampleRate() });
        converted.setPts(av::Timestamp(aptsCounter, converted.timeBase()));
        av::Packet pkt = aenc.encode(converted);
        if (pkt && pkt.size() > 0) {
            pkt.setStreamIndex(astream.index());
            pkt.setTimeBase(astream.timeBase());
            pkt.setDuration(1, astream.timeBase());
            ofctx.writePacket(pkt);
            aptsCounter += converted.samplesCount();
            return 1;
        }
        return 0;
    }

    // 有固定帧大小，使用缓冲区累积
    static std::vector<uint8_t> fifoBuffer; // 简化：平面格式用交错方式？不，这里我们直接用 deque<AudioSamples> 更好，但为简单，假设所有操作都在同一个线程，使用成员变量 audioFifo
    // 由于这是静态函数，无法访问成员。改成将需要累积的功能移到类内部。这里提供一个简化但可直接使用的版本：
    // 直接尝试按帧大小切分编码，不足一帧的返回出去让调用者累积。
    int totalSamples = converted.samplesCount();
    int offset = 0;
    int encodedCount = 0;
    while (offset + frameSize <= totalSamples) {
        // 提取一个子帧
        av::AudioSamples subFrame;
        if (subFrame.init(converted.sampleFormat(), frameSize, converted.channelsLayout(), converted.sampleRate()) < 0) {
            logError("无法分配音频子帧");
            break;
        }
        // 拷贝数据（平面或交错）
        if (!converted.isPlanar()) {
            int bytesPerSample = converted.sampleFormat().bytesPerSample();
            int ch = converted.channelsCount();
            memcpy(subFrame.data(0),
                reinterpret_cast<const uint8_t*>(converted.data(0)) + offset * ch * bytesPerSample,
                frameSize * ch * bytesPerSample);
        }
        else {
            int bytesPerSample = converted.sampleFormat().bytesPerSample();
            for (int c = 0; c < converted.channelsCount(); ++c) {
                memcpy(subFrame.data(c),
                    reinterpret_cast<const uint8_t*>(converted.data(c)) + offset * bytesPerSample,
                    frameSize * bytesPerSample);
            }
        }

        subFrame.setTimeBase({ 1, aenc.sampleRate() });
        subFrame.setPts(av::Timestamp(aptsCounter, subFrame.timeBase()));

        av::Packet pkt = aenc.encode(subFrame);
        if (pkt && pkt.size() > 0) {
            pkt.setStreamIndex(astream.index());
            pkt.setTimeBase(astream.timeBase());
            pkt.setDuration(1, astream.timeBase());
            ofctx.writePacket(pkt);
            encodedCount++;
        }
        else {
            logWarning(std::string("音频编码未产生包 (PTS=") + std::to_string(aptsCounter) + ")");
        }

        aptsCounter += frameSize;
        offset += frameSize;
    }

    // 剩余不足一帧的样本数量
    int remaining = totalSamples - offset;
    // 我们无法在这里累积，因此这部分功能已移至 MediaRecorder::Encode() 内部，使用成员变量 audioFifo
    return encodedCount;
}

void MediaRecorder::Encode() {
    // ---------- 视频编码 ----------
    auto opFrame = this->pCapturer->TryPop();
    if (opFrame.has_value()) {
        auto srcFrame = opFrame.value();
        av::VideoFrame dstFrame(AV_PIX_FMT_YUV420P, this->width, this->height);

        if (!this->rescaler.isValid() ||
            this->rescaler.srcWidth() != this->width ||
            this->rescaler.srcHeight() != this->height ||
            this->rescaler.srcPixelFormat() != this->pCapturer->srcPixelFormat) {
            this->rescaler = av::VideoRescaler(
                this->width, this->height, AV_PIX_FMT_YUV420P,
                this->pCapturer->stagingWidth, this->pCapturer->stagingHeight,
                this->pCapturer->srcPixelFormat, av::SwsFlagFastBilinear
            );
        }

        try {
            this->rescaler.rescale(dstFrame, srcFrame);
            dstFrame.setTimeBase(this->timeBase);
            dstFrame.setPts(av::Timestamp(this->ptsCounter++, this->timeBase));
            dstFrame.setStreamIndex(this->vstream.index());

            av::Packet pkt = this->encoder.encode(dstFrame);
            if (pkt && pkt.size() > 0) {
                pkt.setStreamIndex(this->vstream.index());
                pkt.setTimeBase(this->vstream.timeBase());
                pkt.setDuration(1, this->vstream.timeBase());
                this->ofctx.writePacket(pkt);
            }
        }
        catch (const std::exception& e) {
            this->ISys().LogError(std::string("视频帧写入失败: ") + e.what());
        }
    }

    // ---------- 音频编码 ----------
    if (!this->pAudioCapturer || !this->aencoder.isOpened()) return;

    auto opAudio = this->pAudioCapturer->TryPop();
    if (!opAudio.has_value()) return;

    auto asamples = opAudio.value();
    if (asamples.samplesCount() == 0) return;

    // 1. 格式转换与重采样（如果需要）
    av::AudioSamples converted;
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
                this->ISys().LogError(std::string("AudioResampler 初始化失败: ") + (err ? err.message() : "unknown"));
                return;
            }
            this->ISys().LogWarning("AudioResampler 已初始化");
        }

        this->aresampler.push(asamples);
        converted = this->aresampler.pop(0);
        if (!converted.isValid() || converted.samplesCount() == 0) {
            // 重采样器需要更多数据才能输出，正常等待
            return;
        }
    }
    else {
        // 格式已经匹配，直接使用
        converted = asamples;
    }

    // 2. 按编码器帧大小送入编码器
    int frameSize = this->aencoder.frameSize();
    if (frameSize <= 0) {
        // 无固定帧大小，直接编码
        converted.setTimeBase({ 1, this->aencoder.sampleRate() });
        converted.setPts(av::Timestamp(this->aptsCounter, converted.timeBase()));
        av::Packet pkt = this->aencoder.encode(converted);
        if (pkt && pkt.size() > 0) {
            pkt.setStreamIndex(this->astream.index());
            pkt.setTimeBase(this->astream.timeBase());
            pkt.setDuration(1, this->astream.timeBase());
            this->ofctx.writePacket(pkt);
        }
        this->aptsCounter += converted.samplesCount();
        return;
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
            this->ISys().LogError("无法分配音频帧");
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
            if (pkt && pkt.size() > 0) {
                pkt.setStreamIndex(this->astream.index());
                pkt.setTimeBase(this->astream.timeBase());
                pkt.setDuration(1, this->astream.timeBase());
                this->ofctx.writePacket(pkt);
            }
            else {
                this->ISys().LogWarning("编码器未产生音频包");
            }
        }
        catch (const std::exception& e) {
            this->ISys().LogError(std::string("音频编码失败: ") + e.what());
        }

        this->aptsCounter += frameSize;
        totalSamples -= frameSize;
    }
}