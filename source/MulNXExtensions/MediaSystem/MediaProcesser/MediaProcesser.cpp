#include "MediaProcesser.hpp"
#include "AudioModify/AudioModify.hpp"

bool MediaProcesser::Init() {
    (*this)
        .SubscribeAsync("Media/Concat/Begin")
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
    case "Media/Concat/Begin"_hash: {
        auto path = msg.asp.get<MulNX::NetExt>()->str1;
        this->BeginConcat(path);
        break;
    }
    case "Media/Concat/Add"_hash: {
        auto path = msg.asp.get<MulNX::NetExt>()->str1;
        this->AddConcat(path);
        break;
    }
    case "Media/Concat/End"_hash: {
        this->EndConcat();
        break;
    }
    }
}

void MediaProcesser::BeginConcat(const fs::path& target) {
    this->concatInputs.clear();
    this->concatTarget = target;
    this->concatActive = true;
    this->LogInfo(std::string("开始合并, 目标=") + (target.empty() ? "(未指定)" : target.string()));
}

void MediaProcesser::AddConcat(const fs::path& add) {
    if (!this->concatActive) {
        this->LogWarning("未处于合并状态，忽略 AddConcat");
        return;
    }
    this->concatInputs.push_back(add);
    this->LogInfo(std::string("已加入文件: ") + add.string());
}

void MediaProcesser::EndConcat() {
    std::vector<fs::path> inputs;
    fs::path output;
    if (!this->concatActive) {
        this->LogWarning("EndConcat 在非合并状态被调用，忽略");
        return;
    }
    inputs = this->concatInputs;
    output = this->concatTarget;
    this->concatActive = false;
    this->concatInputs.clear();
    this->concatTarget.clear();

    if (inputs.empty()) {
        this->LogWarning("没有要合并的输入文件");
        return;
    }
    if (output.empty()) {
        this->LogWarning("未指定输出文件，合并取消");
        return;
    }
    try {
        this->DoConcat(inputs, output);
    }
    catch (const std::exception& e) {
        this->LogError(std::string("合并失败: ") + e.what());
    }
}
void MediaProcesser::DoConcat(const std::vector<fs::path>& inputs, const fs::path& output,
    const std::string& audioEncoderName) {
    for (const auto& input : inputs) {
        std::error_code ec;
        if (fs::exists(output, ec) && !ec && fs::equivalent(input, output, ec) && !ec) {
            throw std::runtime_error("输出路径不能覆盖输入文件: " + input.string());
        }
    }

    std::optional<AudioEncoderConfig> audioConfig;
    for (const auto& input : inputs) {
        av::FormatContext probe;
        probe.openInput(input.string());
        probe.findStreamInfo();
        for (size_t i = 0; i < probe.streamsCount(); ++i) {
            auto stream = probe.stream(i);
            if (!stream.isAudio()) {
                continue;
            }
            audioConfig = CreateAudioEncoderConfig(stream, audioEncoderName);
            break;
        }
        if (audioConfig.has_value()) {
            break;
        }
    }

    av::FormatContext outCtx;
    outCtx.openOutput(output.string());

    std::vector<int> firstInputToOutput;
    std::vector<AVMediaType> firstInputMediaTypes;
    std::vector<av::Stream> outStreams;
    int audioOutputIndex = -1;
    int64_t timelineOffsetUs = 0;
    bool headerWritten = false;
    std::vector<int64_t> lastDts;

    for (size_t fileIdx = 0; fileIdx < inputs.size(); ++fileIdx) {
        av::FormatContext inCtx;
        inCtx.openInput(inputs[fileIdx].string());
        inCtx.findStreamInfo();

        int videoInputIndex = -1;
        int audioInputIndex = -1;
        for (size_t i = 0; i < inCtx.streamsCount(); ++i) {
            auto stream = inCtx.stream(i);
            if (videoInputIndex < 0 && stream.isVideo()) {
                videoInputIndex = static_cast<int>(i);
            }
            if (audioInputIndex < 0 && stream.isAudio()) {
                audioInputIndex = static_cast<int>(i);
            }
        }
        if (videoInputIndex < 0) {
            throw std::runtime_error("输入文件没有视频流: " + inputs[fileIdx].string());
        }
        const int64_t segmentDurationUs = GetVideoDurationUs(inCtx, inCtx.stream(videoInputIndex));
        const int64_t segmentSamples = audioConfig.has_value()
            ? GetDurationSamples(av::Timestamp(segmentDurationUs, MicrosecondTimeBase), audioConfig->sampleRate)
            : 0;
        const int64_t segmentOffsetSamples = audioConfig.has_value()
            ? GetDurationSamples(av::Timestamp(timelineOffsetUs, MicrosecondTimeBase), audioConfig->sampleRate)
            : 0;

        if (fileIdx == 0) {
            firstInputToOutput.assign(inCtx.streamsCount(), -1);
            firstInputMediaTypes.reserve(inCtx.streamsCount());
            for (size_t i = 0; i < inCtx.streamsCount(); ++i) {
                auto inputStream = inCtx.stream(i);
                firstInputMediaTypes.push_back(inputStream.mediaType());
                if (inputStream.isAudio()) {
                    continue;
                }
                auto outputStream = outCtx.addStream();
                outputStream.setCodecParameters(inputStream.codecParameters());
                outputStream.setTimeBase(inputStream.timeBase());
                firstInputToOutput[i] = static_cast<int>(outStreams.size());
                outStreams.push_back(outputStream);
            }

            if (audioConfig.has_value()) {
                auto encoder = CreateAudioEncoder(*audioConfig);
                auto outputStream = outCtx.addStream(encoder);
                outputStream.setTimeBase({ 1, audioConfig->sampleRate });
                audioOutputIndex = static_cast<int>(outStreams.size());
                outStreams.push_back(outputStream);
            }

            outCtx.writeHeader();
            headerWritten = true;
            lastDts.assign(outStreams.size(), std::numeric_limits<int64_t>::min());
        }

        std::optional<av::AudioDecoderContext> audioDecoder;
        std::optional<AudioSegmentWriter> audioWriter;
        if (audioConfig.has_value() && audioInputIndex >= 0) {
            audioDecoder.emplace(inCtx.stream(audioInputIndex));
            audioDecoder->open();
            audioWriter.emplace(outCtx, outStreams[audioOutputIndex], &*audioDecoder,
                *audioConfig, segmentSamples, segmentOffsetSamples);
        }
        else if (audioConfig.has_value()) {
            this->LogWarning(std::string("输入文件没有音频流，将补充静音: ") + inputs[fileIdx].string());
        }

        while (true) {
            av::Packet packet = inCtx.readPacket();
            if (!packet || packet.isNull()) {
                break;
            }

            const int inputStreamIndex = packet.streamIndex();
            if (inputStreamIndex == audioInputIndex) {
                if (audioWriter.has_value()) {
                    audioWriter->ProcessPacket(packet);
                }
                continue;
            }

            if (inputStreamIndex < 0 || static_cast<size_t>(inputStreamIndex) >= inCtx.streamsCount()) {
                continue;
            }

            auto inputStream = inCtx.stream(inputStreamIndex);
            int outputStreamIndex = -1;
            if (fileIdx == 0) {
                outputStreamIndex = firstInputToOutput[inputStreamIndex];
            }
            else {
                size_t ordinal = 0;
                for (int i = 0; i < inputStreamIndex; ++i) {
                    if (!inCtx.stream(i).isAudio() &&
                        inCtx.stream(i).mediaType() == inputStream.mediaType()) {
                        ++ordinal;
                    }
                }
                for (size_t i = 0; i < firstInputToOutput.size(); ++i) {
                    if (firstInputToOutput[i] < 0 || firstInputMediaTypes[i] != inputStream.mediaType()) {
                        continue;
                    }
                    if (ordinal == 0) {
                        outputStreamIndex = firstInputToOutput[i];
                        break;
                    }
                    --ordinal;
                }
            }
            if (outputStreamIndex < 0 || static_cast<size_t>(outputStreamIndex) >= outStreams.size()) {
                continue;
            }

            auto& outputStream = outStreams[outputStreamIndex];
            int64_t pts = packet.pts().isValid()
                ? inputStream.timeBase().rescale(packet.pts().timestamp(inputStream.timeBase()), outputStream.timeBase())
                : 0;
            int64_t dts = packet.dts().isValid()
                ? inputStream.timeBase().rescale(packet.dts().timestamp(inputStream.timeBase()), outputStream.timeBase())
                : 0;
            const int64_t offset = MicrosecondTimeBase.rescale(timelineOffsetUs, outputStream.timeBase());
            if (packet.pts().isValid()) {
                pts += offset;
                packet.setPts(av::Timestamp(pts, outputStream.timeBase()));
            }
            if (packet.dts().isValid()) {
                dts += offset;
                if (dts <= lastDts[outputStreamIndex]) {
                    const int64_t correction = lastDts[outputStreamIndex] + 1 - dts;
                    dts += correction;
                    if (packet.pts().isValid()) {
                        pts += correction;
                        packet.setPts(av::Timestamp(pts, outputStream.timeBase()));
                    }
                }
                packet.setDts(av::Timestamp(dts, outputStream.timeBase()));
                lastDts[outputStreamIndex] = dts;
            }

            packet.setDuration(static_cast<int>(
                inputStream.timeBase().rescale(packet.duration(), outputStream.timeBase())),
                outputStream.timeBase());
            packet.setStreamIndex(outputStreamIndex);
            packet.setTimeBase(outputStream.timeBase());
            outCtx.writePacket(packet);
        }

        if (audioWriter.has_value()) {
            audioWriter->Finish();
        }
        else if (audioConfig.has_value()) {
            AudioSegmentWriter silenceWriter(outCtx, outStreams[audioOutputIndex], nullptr,
                *audioConfig, segmentSamples, segmentOffsetSamples);
            silenceWriter.Finish();
        }

        timelineOffsetUs += segmentDurationUs;
    }

    if (!headerWritten) {
        throw std::runtime_error("没有可写入的输入流");
    }
    outCtx.writeTrailer();
    outCtx.close();
    this->LogSucc(std::string("合并完成: ") + output.string());
}