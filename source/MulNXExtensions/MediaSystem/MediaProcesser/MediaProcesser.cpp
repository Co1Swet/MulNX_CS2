#include "MediaProcesser.hpp"
#include "AudioModify/AudioModify.hpp"

bool MediaProcesser::Init() {
    (*this)
        .SubscribeAsync("Media/Concat/Add")
        .SubscribeAsync("Media/Concat/End");

    this->SendTask("Update", "AVEncoding", [this]() {
        this->Update();
        return true;
        });

    return true;
}

void MediaProcesser::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Media/Concat/Add"_hash: {
        auto pExt = msg.asp.get<MulNX::NetExt>();
        this->AddConcat(pExt->str1, pExt->str2);
        break;
    }
    case "Media/Concat/End"_hash: {
        auto pExt = msg.asp.get<MulNX::NetExt>();
        this->EndConcat(pExt->str1);
        break;
    }
    }
}

void MediaProcesser::AddConcat(const fs::path& concatTarget, const fs::path& add) {
    this->infos[concatTarget].segments.push_back({ add,{} });
    this->LogInfo(std::string("已加入文件: ") + add.string());
}

void MediaProcesser::EndConcat(const fs::path& concatTarget) {
    std::vector<fs::path> inputs;

    auto it = this->infos.find(concatTarget);
    if (it == this->infos.end())return;
    auto& [target, info] = *it;
    for (auto& s : info.segments) {
        inputs.push_back(s.v);
    }

    auto output = target;

    if (inputs.empty()) {
        this->LogWarning("没有要合并的输入文件");
        return;
    }
    if (output.empty()) {
        this->LogWarning("未指定输出文件，合并取消");
        return;
    }
    while (this->pMediaState->IsBusy()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    try {
        for (const auto& input : inputs) {
            std::error_code ec;
            if (fs::exists(output, ec) && !ec && fs::equivalent(input, output, ec) && !ec) {
                throw std::runtime_error("输出路径不能覆盖输入文件: " + input.string());
            }
        }

        this->DoConcat(inputs, output);
    }
    catch (const std::exception& e) {
        this->LogError(std::string("合并失败: ") + e.what());
    }
}

class ConcatSession {
public:
    ConcatSession(av::FormatContext& outCtx, const AudioEncoderConfig* audioConfig)
        : outCtx_(outCtx), audioConfig_(audioConfig) {}

    // 喂入一段输入。首段自动建输出流 + writeHeader（lazy header）
    void ProcessNextSegment(av::FormatContext& inCtx, const fs::path& path);

    // 收尾：writeTrailer（若曾写过 header）
    void Close();

private:
    // 一段输入内的流映射：输入流索引 -> 输出流索引（-1 表示丢弃）
    struct StreamMapping {
        int videoInputIndex = -1;
        int audioInputIndex = -1;
        std::vector<int> inputToOutput;
    };

    StreamMapping BuildMapping(av::FormatContext& inCtx) const;
    int FindOutputByOrdinal(AVMediaType type, int ordinal) const;
    void OpenFirstSegment(av::FormatContext& inCtx, const fs::path& path);
    void ProcessSegment(av::FormatContext& inCtx, const fs::path& path,
        const StreamMapping& map);
    void ProcessRemuxPacket(av::Packet& packet, const av::Stream& inStream,
        int outStreamIndex, int64_t offsetUs);
    void FinishAudio(std::optional<AudioSegmentWriter>& writer,
        int64_t segmentSamples, int64_t segmentOffsetSamples);

    // ---- 数据 ----
    av::FormatContext& outCtx_;
    const AudioEncoderConfig* audioConfig_;   // 可为空：无音频输入
    std::vector<av::Stream> outStreams_;
    std::vector<AVMediaType> outStreamTypes_;  // 与 outStreams_ 平行
    std::vector<int64_t> lastDts_;            // 与 outStreams_ 平行
    int audioOutStreamIndex_ = -1;
    int64_t timelineOffsetUs_ = 0;
    bool headerWritten_ = false;
};

// ---------------- Mapping ----------------

ConcatSession::StreamMapping
ConcatSession::BuildMapping(av::FormatContext& inCtx) const {
    StreamMapping map;
    map.inputToOutput.assign(inCtx.streamsCount(), -1);

    // 找第一个视频 / 音频输入
    for (size_t i = 0; i < inCtx.streamsCount(); ++i) {
        const auto s = inCtx.stream(i);
        if (s.isVideo() && map.videoInputIndex < 0)
            map.videoInputIndex = static_cast<int>(i);
        if (s.isAudio() && map.audioInputIndex < 0)
            map.audioInputIndex = static_cast<int>(i);
    }

    // 非音频流：按"同类型序号"匹配输出流
    // 注意：AVMediaType 存在负值（AVMEDIA_TYPE_UNKNOWN / AVMEDIA_TYPE_DATA = -1），
    // 不能用固定大小数组做下标，改用 map。
    std::unordered_map<int, int> ordinalByType;
    for (size_t i = 0; i < inCtx.streamsCount(); ++i) {
        const auto s = inCtx.stream(i);
        if (s.isAudio()) continue;
        const int mt = static_cast<int>(s.mediaType());
        if (mt < 0) continue;    // 未识别类型，丢弃
        const int ordinal = ordinalByType[mt]++;
        map.inputToOutput[i] = FindOutputByOrdinal(s.mediaType(), ordinal);
    }
    return map;
}

int ConcatSession::FindOutputByOrdinal(AVMediaType type, int ordinal) const {
    for (size_t i = 0; i < outStreams_.size(); ++i) {
        if (outStreamTypes_[i] != type) continue;
        if (ordinal == 0) return static_cast<int>(i);
        --ordinal;
    }
    return -1;
}

// ---------------- 首段：建流 + writeHeader ----------------

void ConcatSession::OpenFirstSegment(av::FormatContext& inCtx,
    const fs::path& path) {
    bool hasVideo = false;
    for (size_t i = 0; i < inCtx.streamsCount(); ++i) {
        if (inCtx.stream(i).isVideo()) { hasVideo = true; break; }
    }
    if (!hasVideo) {
        throw std::runtime_error("首个输入文件没有视频流，无法建立输出: "
            + path.string());
    }

    // 非音频流：直接 remux，复用源 codecParameters
    for (size_t i = 0; i < inCtx.streamsCount(); ++i) {
        const auto inStream = inCtx.stream(i);
        if (inStream.isAudio()) continue;

        auto outStream = outCtx_.addStream();
        outStream.setCodecParameters(inStream.codecParameters());
        outStream.setTimeBase(inStream.timeBase());
        outStreams_.push_back(outStream);
        outStreamTypes_.push_back(inStream.mediaType());
    }

    // 音频：统一重编码
    if (audioConfig_) {
        auto encoder = CreateAudioEncoder(*audioConfig_);
        auto outStream = outCtx_.addStream(encoder);
        outStream.setTimeBase({ 1, audioConfig_->sampleRate });
        audioOutStreamIndex_ = static_cast<int>(outStreams_.size());
        outStreams_.push_back(outStream);
        outStreamTypes_.push_back(AVMEDIA_TYPE_AUDIO);
    }

    outCtx_.writeHeader();
    headerWritten_ = true;
    lastDts_.assign(outStreams_.size(), std::numeric_limits<int64_t>::min());
}

// ---------------- 时间戳封装 ----------------

void ConcatSession::ProcessRemuxPacket(av::Packet& packet,
    const av::Stream& inStream,
    int outStreamIndex, int64_t offsetUs) {
    auto& outStream = outStreams_[outStreamIndex];
    const auto inTb = inStream.timeBase();
    const auto outTb = outStream.timeBase();
    const int64_t offset = MicrosecondTimeBase.rescale(offsetUs, outTb);

    const bool hasPts = packet.pts().isValid();
    const bool hasDts = packet.dts().isValid();

    int64_t pts = hasPts
        ? inTb.rescale(packet.pts().timestamp(inTb), outTb) + offset
        : 0;
    int64_t dts = hasDts
        ? inTb.rescale(packet.dts().timestamp(inTb), outTb) + offset
        : 0;

    // DTS 单调性修正：把超出部分"平移"回正轨，并同步修正 PTS
    if (hasDts) {
        if (dts <= lastDts_[outStreamIndex]) {
            const int64_t correction = lastDts_[outStreamIndex] + 1 - dts;
            dts += correction;
            if (hasPts) pts += correction;
        }
        packet.setDts(av::Timestamp(dts, outTb));
        lastDts_[outStreamIndex] = dts;
    }
    if (hasPts) {
        packet.setPts(av::Timestamp(pts, outTb));
    }

    packet.setDuration(
        static_cast<int>(inTb.rescale(packet.duration(), outTb)), outTb);
    packet.setStreamIndex(outStreamIndex);
    packet.setTimeBase(outTb);
    outCtx_.writePacket(packet);
}

// ---------------- 单段处理 ----------------

void ConcatSession::ProcessSegment(av::FormatContext& inCtx,
    const fs::path& path,
    const StreamMapping& map) {
    const int64_t segmentDurationUs =
        GetVideoDurationUs(inCtx, inCtx.stream(map.videoInputIndex));

    const int64_t segmentSamples = audioConfig_
        ? GetDurationSamples(av::Timestamp(segmentDurationUs, MicrosecondTimeBase),
            audioConfig_->sampleRate)
        : 0;
    const int64_t segmentOffsetSamples = audioConfig_
        ? GetDurationSamples(av::Timestamp(timelineOffsetUs_, MicrosecondTimeBase),
            audioConfig_->sampleRate)
        : 0;

    // 音频 writer（有源才建；无源 FinishAudio 会补静音）
    std::optional<av::AudioDecoderContext> audioDecoder;
    std::optional<AudioSegmentWriter> audioWriter;
    if (audioConfig_ && map.audioInputIndex >= 0) {
        audioDecoder.emplace(inCtx.stream(map.audioInputIndex));
        audioDecoder->open();
        audioWriter.emplace(outCtx_, outStreams_[audioOutStreamIndex_],
            &*audioDecoder, *audioConfig_,
            segmentSamples, segmentOffsetSamples);
    }

    // packet 循环
    while (true) {
        auto packet = inCtx.readPacket();
        if (!packet || packet.isNull()) break;

        const int inIdx = packet.streamIndex();

        // 音频 → writer（多余音频流直接丢弃）
        if (inIdx == map.audioInputIndex) {
            if (audioWriter.has_value()) {
                audioWriter->ProcessPacket(packet);
            }
            continue;
        }
        if (inIdx < 0 ||
            static_cast<size_t>(inIdx) >= map.inputToOutput.size()) {
            continue;
        }
        const int outIdx = map.inputToOutput[inIdx];
        if (outIdx < 0) continue;

        ProcessRemuxPacket(packet, inCtx.stream(inIdx), outIdx, timelineOffsetUs_);
    }

    FinishAudio(audioWriter, segmentSamples, segmentOffsetSamples);

    timelineOffsetUs_ += segmentDurationUs;
}

void ConcatSession::FinishAudio(std::optional<AudioSegmentWriter>& writer,
    int64_t segmentSamples,
    int64_t segmentOffsetSamples) {
    if (writer.has_value()) {
        writer->Finish();
        return;
    }
    if (!audioConfig_) return;

    // 无音频源 → 补静音
    AudioSegmentWriter silence(outCtx_, outStreams_[audioOutStreamIndex_],
        nullptr, *audioConfig_,
        segmentSamples, segmentOffsetSamples);
    silence.Finish();
}

// ---------------- 对外入口 ----------------

void ConcatSession::ProcessNextSegment(av::FormatContext& inCtx,
    const fs::path& path) {
    // 关键顺序：先确保输出流全部就位（首段会建流 + writeHeader），
    // 再基于 outStreams_ 构造本段的 input->output 映射。
    // 反之会让首段 mapping 全为 -1，所有视频包被丢弃。
    if (!headerWritten_) {
        OpenFirstSegment(inCtx, path);
    }

    const auto map = BuildMapping(inCtx);
    if (map.videoInputIndex < 0) {
        throw std::runtime_error("输入文件没有视频流: " + path.string());
    }
    ProcessSegment(inCtx, path, map);
}

void ConcatSession::Close() {
    if (!headerWritten_) {
        throw std::runtime_error("没有任何可写入的输入流");
    }
    outCtx_.writeTrailer();
    outCtx_.close();
}

void MediaProcesser::DoConcat(const std::vector<fs::path>& inputs,
    const fs::path& output,
    const std::string& audioEncoderName) {
    if (inputs.empty()) {
        throw std::runtime_error("没有要合并的输入文件");
    }

    // 1) 输出不得覆盖任意输入
    for (const auto& input : inputs) {
        std::error_code ec;
        if (fs::exists(output, ec) && !ec &&
            fs::equivalent(input, output, ec) && !ec) {
            throw std::runtime_error("输出路径不能覆盖输入文件: "
                + input.string());
        }
    }

    // 2) 探测统一音频编码配置（取首个有音频的文件）
    std::optional<AudioEncoderConfig> audioConfig;
    for (const auto& input : inputs) {
        av::FormatContext probe;
        probe.openInput(input.string());
        probe.findStreamInfo();
        for (size_t i = 0; i < probe.streamsCount(); ++i) {
            if (probe.stream(i).isAudio()) {
                audioConfig =
                    CreateAudioEncoderConfig(probe.stream(i), audioEncoderName);
                break;
            }
        }
        if (audioConfig.has_value()) break;
    }
    // 3) 打开输出
    av::FormatContext outCtx;
    outCtx.openOutput(output.string());
    // 4) 逐段喂入
    ConcatSession session(outCtx, audioConfig ? &*audioConfig : nullptr);
    for (const auto& input : inputs) {
        av::FormatContext inCtx;
        inCtx.openInput(input.string());
        inCtx.findStreamInfo();
        session.ProcessNextSegment(inCtx, input);
    }
    // 5) 收尾
    session.Close();

    this->LogSucc(std::string("合并完成: ") + output.string());
}