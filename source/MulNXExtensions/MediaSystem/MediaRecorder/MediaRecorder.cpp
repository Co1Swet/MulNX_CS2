#include "MediaRecorder.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/MediaSystem/Videos/VCD3D11Manager/VCD3D11Manager.hpp>
#include <MulNXExtensions/MediaSystem/AudioCapturer/AudioCapturer.hpp>
#include <MulNXExtensions/MediaSystem/AEncodeHelper/AEncodeHelper.hpp>
#include <MulNXExtensions/MediaSystem/VEncodeHelper/VEncodeHelper.hpp>
#include <MulNXExtensions/MediaSystem/MediaParamManager/MediaParamManager.hpp>

void MediaRecorder::CaptureCallback() {
    ImGui::GetBackgroundDrawList()->AddCallback([](const ImDrawList*, const ImDrawCmd* cmd) {
        static_cast<MediaRecorder*>(cmd->UserCallbackData)->PublishSync("MediaSync/PresentCallback"_hash);
        }, this, 0);
}

bool MediaRecorder::Init() {
    this->pAudioCapturer = this->FindModule<AudioCapturer>("AudioCapturer");
    this->pVEncodeHelper = this->FindModule<VEncodeHelper>("VEncodeHelper");
    this->pAEncodeHelper = this->FindModule<AEncodeHelper>("AEncodeHelper");
    this->pVCD3D11Manager = this->FindModule<VCD3D11Manager>("VCD3D11Manager");
    this->pMediaParamManager = this->FindModule<MediaParamManager>("MediaParamManager");
    
    (*this)
        .SubscribeAsync("Media/Record/Start")
        .SubscribeAsync("Media/Record/Stop");

    this->SendTask("Main", "AVEncoding", [this]() { this->Main(); return true; });

    this->UIRegisterBackground("捕获以上所有根触发的UI渲染", [this](auto&&...) {
        return this->CaptureCallback();
        });

    this->UIRegisterCallback("UI.MediaSys", [this](auto&&...) {
        ImGui::Text("录制状态：");
        ImGui::Text(this->recording ? "录制中": "空闲中");
        });

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
    if (this->recording) {
        this->LogWarning("已在录制中");
        return false;
    }

    std::string outFile = pathNoExt + ".mp4";
    int srcW = this->pVCD3D11Manager->srcWidth;
    int srcH = this->pVCD3D11Manager->srcHeight;
    av::PixelFormat srcFmt = this->pVCD3D11Manager->srcAVFormat;
    if (srcW <= 0 || srcH <= 0 || srcFmt == AV_PIX_FMT_NONE) {
        this->LogError("源纹理参数无效"); return false;
    }

    this->PublishSync("MediaSync/Reset"_hash);

    try {
        this->ofctx.openOutput(outFile);

        MulNX::Message SetOnMsg("MediaSync/SetOn"_hash);
        auto&& [rInfo] = SetOnMsg.Access<MulNX::AVStartInfo>();
        rInfo.pOutCtx = &this->ofctx;
        rInfo.startTime = std::chrono::steady_clock::now();
        this->PublishSync(SetOnMsg);

        this->pAEncodeHelper->SetOn(&this->ofctx, this->pAudioCapturer->GetSampleRate());

        this->ofctx.writeHeader();
        this->LogInfo(std::format("输出头已写入, 流数={}", this->ofctx.streamsCount()));

        auto& rp = *this->pMediaParamManager;
        this->LogSucc(std::format("开始录制: {} ({}x{})",
            outFile,
            rp.width > 0 ? rp.width : srcW,
            rp.height > 0 ? rp.height : srcH,
            rp.targetFPS > 0 ? std::to_string(rp.targetFPS) + "fps " : ""
        ));

        this->recording = true;
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
    if (!this->recording) return;
    this->Encode();
}

void MediaRecorder::Encode() {
    std::vector<av::Packet> packets;

    while (auto p = this->pVEncodeHelper->Encode()) {
        packets.push_back(std::move(*p));
    }
        
    while (auto a = this->pAudioCapturer->TryPop()) {
        if (a->samplesCount() > 0)
            if (auto p = this->pAEncodeHelper->Encode(std::move(*a)))
                packets.push_back(std::move(*p));
    }

    if (packets.empty()) return;

    for (auto& pkt : packets) {
        try { this->ofctx.writePacket(pkt); }
        catch (const std::exception& e) {
            this->LogWarning(std::format("写包失败: {}", e.what()));
        }
    }
}

bool MediaRecorder::StopRecording() {
    if (!this->recording) return false;
    this->recording = false;

    this->PublishSync("MediaSync/SetOff"_hash);

    try {
        while (auto p = this->pVEncodeHelper->Encode()) {
            this->ofctx.writePacket(*p);
        }                
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

    this->PublishSync("MediaSync/Reset"_hash);

    this->ofctx.close();
    this->LogSucc("录制结束");

    return true;
}