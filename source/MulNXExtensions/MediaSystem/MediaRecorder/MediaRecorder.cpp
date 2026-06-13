#include "MediaRecorder.hpp"
#include <MulNXExtensions/MediaSystem/VideoCapturer/VideoCapturer.hpp>
#include <MulNXExtensions/MediaSystem/AudioCapturer/AudioCapturer.hpp>
#include <MulNXExtensions/MediaSystem/AEncodeHelper/AEncodeHelper.hpp>
#include <MulNXExtensions/MediaSystem/VEncodeHelper/VEncodeHelper.hpp>

bool MediaRecorder::Init() {
    this->pVideoCapturer = this->Core->ModuleManager()->FindModule<VideoCapturer>("VideoCapturer");
    this->pAudioCapturer = this->Core->ModuleManager()->FindModule<AudioCapturer>("AudioCapturer");
    this->pVEncodeHelper = this->Core->ModuleManager()->FindModule<VEncodeHelper>("VEncodeHelper");
    this->pAEncodeHelper = this->Core->ModuleManager()->FindModule<AEncodeHelper>("AEncodeHelper");

    this->dirVedios = this->ISys().Path()->PathGetForShared("Vedios");

    this->ISys()
        .SubscribeAsync("Media/Record/Start")
        .SubscribeAsync("Media/Record/Stop");

    this->ISys().SendTask("Main", "Media", [this]() {
        this->Main();
        return true;
        });

    return true;
}

void MediaRecorder::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Media/Record/Start"_hash: {
        auto outFile = this->dirVedios / "record.mp4";
        this->StartRecording(outFile.string(), 1920, 1080);
        break;
    }
    case "Media/Record/Stop"_hash: {
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

    if (!this->pAudioCapturer || !this->pVideoCapturer || !this->pAEncodeHelper || !this->pVEncodeHelper) {
        this->ISys().LogError("录制启动失败：缺少音视频模块");
        return false;
    }

    // 清理旧缓存，保持录制起点对齐
    this->pAudioCapturer->ClearBuffer();
    this->pVideoCapturer->ClearBuffer();
    this->pVideoCapturer->Reset();
    this->pAEncodeHelper->Reset();

    try {
        this->ofctx.openOutput(filename);
        this->recordStartTime = std::chrono::steady_clock::now();
        this->pVideoCapturer->StartCapture(this->recordStartTime);

        this->pVEncodeHelper->SetOn(&this->ofctx, w, h, this->timeBase);
        this->pAEncodeHelper->SetOn(&this->ofctx, this->pAudioCapturer->GetSampleRate());

        // 写文件头（即使只有视频流）
        this->ofctx.writeHeader();
        this->ISys().LogWarning(std::format("输出文件头已写入，流数量={}", this->ofctx.streamsCount()));

        this->runFlag1 = true;
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

    this->Encode();
}

static int64_t TimestampInMicroseconds(const av::Timestamp &ts) {
    return ts.isValid() ? ts.timestamp({1, 1000000}) : std::numeric_limits<int64_t>::max();
}

void MediaRecorder::Encode() {
    std::vector<av::Packet> packets;

    // 视频编码
    while (auto opFrame = this->pVideoCapturer->TryPop()) {
        this->pVEncodeHelper->CheckRescaler(
            this->pVideoCapturer->stagingWidth, this->pVideoCapturer->stagingHeight,
            this->pVideoCapturer->srcPixelFormat);

        if (auto pkt = this->pVEncodeHelper->Encode(*opFrame)) {
            packets.push_back(std::move(*pkt));
        }
    }

    // 音频编码
    while (auto opAudio = this->pAudioCapturer->TryPop()) {
        if (opAudio->samplesCount() > 0) {
            if (auto pkt = this->pAEncodeHelper->Encode(std::move(*opAudio))) {
                packets.push_back(std::move(*pkt));
            }
        }
    }

    if (packets.empty()) {
        return;
    }

    std::sort(packets.begin(), packets.end(), [](const av::Packet &left, const av::Packet &right) {
        return TimestampInMicroseconds(left.pts()) < TimestampInMicroseconds(right.pts());
    });

    for (auto &pkt : packets) {
        try {
            this->ofctx.writePacket(pkt);
        }
        catch (const std::exception& e) {
            this->ISys().LogWarning(std::format("写入 packet 失败: {}, 跳过该包", e.what()));
        }
    }
}

bool MediaRecorder::StopRecording() {
    if (!this->runFlag1) return false;

    this->runFlag1 = false;
    this->pVideoCapturer->StopCapture();

    try {
        // Drain pending captured video frames before flushing the encoder.
        while (auto opFrame = this->pVideoCapturer->TryPop()) {
            if (auto pkt = this->pVEncodeHelper->Encode(*opFrame)) {
                this->ofctx.writePacket(*pkt);
            }
        }

        while (auto pkt = this->pVEncodeHelper->TrySetOff()) {
            this->ofctx.writePacket(*pkt);
        }

        while (auto opAudio = this->pAudioCapturer->TryPop()) {
            if (opAudio->samplesCount() > 0) {
                if (auto pkt = this->pAEncodeHelper->Encode(std::move(*opAudio))) {
                    this->ofctx.writePacket(*pkt);
                }
            }
        }

        while (auto pkt = this->pAEncodeHelper->TrySetOff()) {
            this->ofctx.writePacket(*pkt);
        }

        this->ofctx.writeTrailer();
    }
    catch (const std::exception& e) {
        this->ISys().LogError(std::string("停止录制出错: ") + e.what());
    }

    this->runFlag1 = false;
    this->pVideoCapturer->StopCapture();
    this->pVideoCapturer->Reset();
    this->pAEncodeHelper->Reset();

    this->ofctx.close();
    return true;
}