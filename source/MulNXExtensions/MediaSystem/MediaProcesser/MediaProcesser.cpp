#include "MediaProcesser.hpp"
#include <limits>

bool MediaProcesser::Init() {
    (*this)
        .SubscribeAsync("Media/Concat/Begin")
        .SubscribeAsync("Media/Concat/Add")
        .SubscribeAsync("Media/Concat/End")
        ;

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

void MediaProcesser::BeginConcat(const std::filesystem::path& target) {
    this->concatInputs.clear();
    this->concatTarget = target;
    this->concatActive = true;
    this->LogInfo(std::string("开始合并, 目标=") + (target.empty() ? "(未指定)" : target.string()));
}

void MediaProcesser::AddConcat(const std::filesystem::path& add) {
    if (!this->concatActive) {
        this->LogWarning("未处于合并状态，忽略 AddConcat");
        return;
    }
    this->concatInputs.push_back(add);
    this->LogInfo(std::string("已加入文件: ") + add.string());
}

void MediaProcesser::EndConcat() {
    std::vector<std::filesystem::path> inputs;
    std::filesystem::path output;
    {
        if (!this->concatActive) {
            this->LogWarning("EndConcat 在非合并状态被调用，忽略");
            return;
        }
        inputs = this->concatInputs;
        output = this->concatTarget;
        this->concatActive = false;
        this->concatInputs.clear();
        this->concatTarget.clear();
    }

    if (inputs.empty()) {
        this->LogWarning("没有要合并的输入文件");
        return;
    }
    if (output.empty()) {
        this->LogWarning("未指定输出文件，合并取消");
        return;
    }

    av::FormatContext outCtx;
    outCtx.openOutput(output.string());

    std::vector<av::Stream> outStreams;
    // 每个输出流单独维护偏移量，避免不同流混用单一偏移导致时间错乱
    std::vector<int64_t> streamOffsets;
    std::vector<int64_t> lastDts;
    av::Rational outTimeBase;
    for (size_t fileIdx = 0; fileIdx < inputs.size(); ++fileIdx) {
        av::FormatContext inCtx;
        inCtx.openInput(inputs[fileIdx].string());
        inCtx.findStreamInfo();

        if (fileIdx == 0) {
            for (size_t i = 0; i < inCtx.streamsCount(); ++i) {
                auto inStream = inCtx.stream(i);
                // 添加输出流（不指定 codec，使用流参数）
                auto outStream = outCtx.addStream();
                // 复制 codec 参数
                outStream.setCodecParameters(inStream.codecParameters());
                outStream.setTimeBase(inStream.timeBase());
                outStreams.push_back(outStream);
            }
            outCtx.writeHeader();
            outTimeBase = outStreams[0].timeBase();
            streamOffsets.assign(outStreams.size(), 0);
            lastDts.assign(outStreams.size(), std::numeric_limits<int64_t>::min());
        }

        // seek 到流起点
        inCtx.seek(0, 0, 0);

        while (true) {
            av::Packet pkt = inCtx.readPacket();
            if (!pkt || pkt.isNull()) break;

            int idx = pkt.streamIndex();
            if (idx < 0 || static_cast<size_t>(idx) >= outStreams.size()) continue;
            auto inStream = inCtx.stream(idx);
            auto& outStream = outStreams[idx];

            // 使用 Timestamp 来转换时间基
            int64_t pts = 0;
            int64_t dts = 0;
            if (pkt.pts().isValid()) pts = pkt.pts().timestamp(inStream.timeBase());
            if (pkt.dts().isValid()) dts = pkt.dts().timestamp(inStream.timeBase());

            // 转换到输出 timebase
            if (pkt.pts().isValid()) pts = inStream.timeBase().rescale(pts, outStream.timeBase());
            if (pkt.dts().isValid()) dts = inStream.timeBase().rescale(dts, outStream.timeBase());

            // 应用对应流偏移
            pts += streamOffsets[idx];
            dts += streamOffsets[idx];

            pkt.setPts(av::Timestamp(pts, outStream.timeBase()));
            pkt.setDts(av::Timestamp(dts, outStream.timeBase()));
            pkt.setDuration(static_cast<int>(inStream.timeBase().rescale(pkt.duration(), outStream.timeBase())), outStream.timeBase());
            pkt.setStreamIndex(idx);
            pkt.setTimeBase(outStream.timeBase());

            // 保证每个输出流的 DTS 单调递增，避免 write 时 EINVAL
            if (pkt.dts().isValid()) {
                int64_t curDts = pkt.dts().timestamp(outStream.timeBase());
                if (curDts <= lastDts[idx]) {
                    curDts = lastDts[idx] + 1;
                    pkt.setDts(av::Timestamp(curDts, outStream.timeBase()));
                    if (pkt.pts().isValid() && pkt.pts().timestamp(outStream.timeBase()) <= curDts) {
                        pkt.setPts(av::Timestamp(curDts + 1, outStream.timeBase()));
                    }
                }
                lastDts[idx] = pkt.dts().timestamp(outStream.timeBase());
            }

            try {
                outCtx.writePacket(pkt);
            }
            catch (const std::exception& e) {
                try {
                    this->LogWarning(std::format("writePacket 失败 idx={} pts={} dts={} err={}，尝试 writePacketDirect",
                        idx,
                        pkt.pts().isValid() ? std::to_string(pkt.pts().timestamp(outStream.timeBase())) : std::string("no"),
                        pkt.dts().isValid() ? std::to_string(pkt.dts().timestamp(outStream.timeBase())) : std::string("no"),
                        e.what()));
                }
                catch (...) {}

                try {
                    outCtx.writePacketDirect(pkt);
                }
                catch (const std::exception& e2) {
                    try {
                        this->LogError(std::format("writePacketDirect 也失败 idx={} err={}", idx, e2.what()));
                    }
                    catch (...) {}
                }
            }
        }

        // 每个流累加该文件时长到对应偏移
        for (size_t si = 0; si < outStreams.size(); ++si) {
            auto inStream = inCtx.stream(si);
            auto durTs = inStream.duration();
            int64_t dur = durTs.isValid() ? durTs.timestamp(inStream.timeBase()) : 0;
            int64_t durOut = inStream.timeBase().rescale(dur, outStreams[si].timeBase());
            streamOffsets[si] += durOut;
        }
    }

    outCtx.writeTrailer();
    this->LogSucc(std::string("合并完成: ") + output.string());
    outCtx.close();
    try {



    }
    catch (const std::exception& e) {
        this->LogError(std::string("合并失败: ") + e.what());
    }
}
