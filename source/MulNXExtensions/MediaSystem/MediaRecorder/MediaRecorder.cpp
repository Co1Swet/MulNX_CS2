#include "MediaRecorder.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/MediaSystem/Videos/VCD3D11Manager/VCD3D11Manager.hpp>
#include <MulNXExtensions/MediaSystem/Videos/VideoCapturer/VideoCapturer.hpp>
#include <MulNXExtensions/MediaSystem/Videos/BufferCopier/BufferCopier.hpp>
#include <MulNXExtensions/MediaSystem/AudioCapturer/AudioCapturer.hpp>
#include <MulNXExtensions/MediaSystem/AEncodeHelper/AEncodeHelper.hpp>
#include <MulNXExtensions/MediaSystem/VEncodeHelper/VEncodeHelper.hpp>
#include <MulNXExtensions/MediaSystem/MediaParamManager/MediaParamManager.hpp>

void MediaRecorder::CaptureCallback() {
    ImGui::GetBackgroundDrawList()->AddCallback([](const ImDrawList*, const ImDrawCmd* cmd) {
        static_cast<MediaRecorder*>(cmd->UserCallbackData)->PublishSync("Hook/BeforePresent"_hash);
        }, this, 0);
}

bool MediaRecorder::Init() {
    this->pVideoCapturer = this->FindModule<VideoCapturer>("VideoCapturer");
    this->pAudioCapturer = this->FindModule<AudioCapturer>("AudioCapturer");
    this->pVEncodeHelper = this->FindModule<VEncodeHelper>("VEncodeHelper");
    this->pAEncodeHelper = this->FindModule<AEncodeHelper>("AEncodeHelper");
    this->pVCD3D11Manager = this->FindModule<VCD3D11Manager>("VCD3D11Manager");
    this->pMediaParamManager = this->FindModule<MediaParamManager>("MediaParamManager");
    this->pBufferCopier = this->FindModule<BufferCopier>("BufferCopier");

    this->dirVideos = this->Path()->PathGetForShared("Videos");
    (*this)
        .SubscribeAsync("Media/Record/Start")
        .SubscribeAsync("Media/Record/Stop");

    this->SendTask("Main", "Media", [this]() { this->Main(); return true; });
    this->UIRegisterBackground("捕获以上所有根触发的UI渲染", [this](auto&&...) { return this->CaptureCallback(); });
    return true;
}

void MediaRecorder::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Media/Record/Start"_hash:
        this->StartRecording(msg.asp.get<MulNX::NetExt>()->str1);
        break;
    case "Media/Record/Stop"_hash:
        this->StopRecording();
        break;
    }
}

bool MediaRecorder::StartRecording(const std::string& pathNoExt) {
    if (this->runFlag1) { this->LogWarning("已在录制中"); return false; }

    RecordParams rp;
    if (this->pMediaParamManager) rp = this->pMediaParamManager->Params();

    std::string outFile = pathNoExt + ".mp4";
    int srcW = this->pVCD3D11Manager->srcWidth;
    int srcH = this->pVCD3D11Manager->srcHeight;
    av::PixelFormat srcFmt = DXGIFormatToAvPixelFormat(this->pVCD3D11Manager->srcDxgiFormat);
    if (srcW <= 0 || srcH <= 0 || srcFmt == AV_PIX_FMT_NONE) {
        this->LogError("源纹理参数无效"); return false;
    }

    this->pBufferCopier->SetCaptureFpsCap(rp.captureFpsCap);
    this->pAudioCapturer->ClearBuffer();
    this->pVideoCapturer->ClearBuffer();
    this->pVideoCapturer->Reset();
    this->pAEncodeHelper->Reset();
    this->pVEncodeHelper->Reset();

    try {
        this->ofctx.openOutput(outFile);
        this->recordStartTime = std::chrono::steady_clock::now();
        this->pBufferCopier->SetRecordStart(this->recordStartTime);

        this->pVEncodeHelper->SetOn(&this->ofctx, rp, srcW, srcH, srcFmt,
            this->pVCD3D11Manager->pReadSideDevice.Get(),
            rp.captureFpsCap > 0 ? rp.captureFpsCap : 0);
        this->pAEncodeHelper->SetOn(&this->ofctx, this->pAudioCapturer->GetSampleRate());

        this->ofctx.writeHeader();
        this->LogInfo(std::format("输出头已写入, 流数={}", this->ofctx.streamsCount()));

        this->pVideoCapturer->StartCapture(this->recordStartTime);
        this->LogSucc(std::format("开始录制: {} ({}x{})", outFile,
            rp.width > 0 ? rp.width : srcW, rp.height > 0 ? rp.height : srcH,
            rp.captureFpsCap > 0 ? std::to_string(rp.captureFpsCap) + "fps " : ""));
        this->runFlag1 = true;
        return true;
    }
    catch (const std::exception& e) {
        this->LogError(std::format("启动失败: {}", e.what()));
        this->ofctx.close();
        return false;
    }
}

void MediaRecorder::Main() {
    this->Update();
    if (!this->runFlag1) return;
    this->Encode();
}

static int64_t PtsUs(const av::Timestamp& ts) {
    return ts.isValid() ? ts.timestamp({ 1,1000000 }) : INT64_MAX;
}

void MediaRecorder::Encode() {
    std::vector<av::Packet> packets;

    while (auto f = this->pVideoCapturer->TryPop())
        if (auto p = this->pVEncodeHelper->Encode(std::move(*f)))
            packets.push_back(std::move(*p));

    while (auto a = this->pAudioCapturer->TryPop()) {
        if (a->samplesCount() > 0)
            if (auto p = this->pAEncodeHelper->Encode(std::move(*a)))
                packets.push_back(std::move(*p));
    }

    if (packets.empty()) return;
    std::sort(packets.begin(), packets.end(),
        [](const av::Packet& l, const av::Packet& r) { return PtsUs(l.dts()) < PtsUs(r.dts()); });

    for (auto& pkt : packets) {
        try { this->ofctx.writePacket(pkt); }
        catch (const std::exception& e) {
            this->LogWarning(std::format("写包失败: {}", e.what()));
        }
    }
}

bool MediaRecorder::StopRecording() {
    if (!this->runFlag1) return false;
    this->runFlag1 = false;
    this->pVideoCapturer->StopCapture();

    try {
        while (auto f = this->pVideoCapturer->TryPop())
            if (auto p = this->pVEncodeHelper->Encode(std::move(*f)))
                this->ofctx.writePacket(*p);
        while (auto p = this->pVEncodeHelper->TrySetOff())
            this->ofctx.writePacket(*p);
        while (auto a = this->pAudioCapturer->TryPop())
            if (a->samplesCount() > 0)
                if (auto p = this->pAEncodeHelper->Encode(std::move(*a)))
                    this->ofctx.writePacket(*p);
        while (auto p = this->pAEncodeHelper->TrySetOff())
            this->ofctx.writePacket(*p);
    }
    catch (const std::exception& e) {
        this->LogWarning(std::format("停止时异常: {}", e.what()));
    }

    this->ofctx.writeTrailer();

    this->pVideoCapturer->Reset();
    this->pAEncodeHelper->Reset();
    this->pVEncodeHelper->Reset();
    this->ofctx.close();

    this->LogSucc("录制结束");

    return true;
}