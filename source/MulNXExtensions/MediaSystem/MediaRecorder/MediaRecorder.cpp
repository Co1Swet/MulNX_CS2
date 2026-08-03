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
    this->pVEncodeHelper = this->FindModule<VEncodeHelper>("VEncodeHelper");
    this->pAEncodeHelper = this->FindModule<AEncodeHelper>("AEncodeHelper");
    this->pVCD3D11Manager = this->FindModule<VCD3D11Manager>("VCD3D11Manager");
    this->pMediaParamManager = this->FindModule<MediaParamManager>("MediaParamManager");
    
    (*this)
        .SubscribeAsync("Media/Record/Start")
        .SubscribeAsync("Media/Record/StartAdvanced")
        .SubscribeAsync("Media/Record/Stop");

    this->SendTask("Main", "AVEncoding", [this]() {
        this->Main();
        return true;
        });

    this->UIRegisterBackground("捕获以上所有根触发的UI渲染", [this](auto&&...) {
        return this->CaptureCallback();
        });

    this->UIRegisterCallback("UI.MediaSys", [this](auto&&...) {
        ImGui::Text("录制状态：");
        switch (this->recordState.load()) {
        case RecordState::Free:
            ImGui::Text("空闲");
            break;
        case RecordState::Recording:
            ImGui::Text("录制中（忙碌）");
            break;
        }
        return true;
        });

    return true;
}

void MediaRecorder::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Media/Record/Start"_hash:
        this->StartRecording(msg.asp.get<MulNX::NetExt>()->str1, false);
        break;
    case "Media/Record/StartAdvanced"_hash:
        this->StartRecording(msg.asp.get<MulNX::NetExt>()->str1, true);
        break;
    case "Media/Record/Stop"_hash:
        this->StopRecording();
        break;
    }
}

bool MediaRecorder::StartRecording(const std::string& pathNoExt, bool advance) {
    if (this->recordState.load() != RecordState::Free) {
        this->LogWarning("已在录制中");
        return false;
    }
    this->pMediaState->advancedMode = advance;

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

        this->ofctx.writeHeader();
        this->LogInfo(std::format("输出头已写入, 流数={}", this->ofctx.streamsCount()));

        auto& rp = *this->pMediaParamManager;
        this->LogSucc(std::format("开始录制: {} ({}x{})",
            outFile,
            rp.width > 0 ? rp.width : srcW,
            rp.height > 0 ? rp.height : srcH,
            rp.targetFPS > 0 ? std::to_string(rp.targetFPS) + "fps " : ""
        ));

        this->pMediaState->MediaSystemGlobalWorkFlag = true;
        this->recordState.store(RecordState::Recording);
        return true;
    }
    catch (const std::exception& e) {
        this->LogError(std::format("启动失败: {}", e.what()));
        this->ofctx.close();
        return false;
    }
}

bool MediaRecorder::StopRecording() {
    this->PublishSync("MediaSync/SetOff"_hash);

    try {
        while (auto p = this->pVEncodeHelper->Encode()) {
            this->ofctx.writePacket(*p);
        }
        while (auto p = this->pVEncodeHelper->TrySetOff()) {
            this->ofctx.writePacket(*p);
        }
            
        while (auto p = this->pAEncodeHelper->Encode()) {
            this->ofctx.writePacket(*p);
        }
        while (auto p = this->pAEncodeHelper->TrySetOff()) {
            this->ofctx.writePacket(*p);
        }
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

void MediaRecorder::Main() {
    this->Update();
    this->Encode();
}

void MediaRecorder::Encode() {
    if (this->recordState.load() != RecordState::Recording) return;
    std::vector<av::Packet> packets;

    while (auto p = this->pVEncodeHelper->Encode()) {
        packets.push_back(std::move(*p));
    }
        
    while (auto p = this->pAEncodeHelper->Encode()) {
        packets.push_back(std::move(*p));
    }

    if (packets.empty()) return;

    for (auto& pkt : packets) {
        try {
            this->ofctx.writePacket(pkt);
        }
        catch (const std::exception& e) {
            this->LogWarning(std::format("写包失败: {}", e.what()));
        }
    }
}

void MediaRecorder::WritePacket() {
    
}