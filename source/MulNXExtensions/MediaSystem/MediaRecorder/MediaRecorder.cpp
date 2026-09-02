#include "MediaRecorder.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <APipeline/AudioCapturer/AudioCapturer.hpp>
#include <APipeline/AEncodeHelper/AEncodeHelper.hpp>
#include <VPipeline/VCD3D11Manager/VCD3D11Manager.hpp>
#include <VPipeline/VEncodeHelper/VEncodeHelper.hpp>
#include <MediaParamManager/MediaParamManager.hpp>

void MediaRecorder::CaptureCallback() {
    ImGui::GetBackgroundDrawList()->AddCallback([](const ImDrawList*, const ImDrawCmd* cmd) {
        static_cast<MediaRecorder*>(cmd->UserCallbackData)->PublishSync("MediaSync/PresentCallback"_hash);
        }, this, 0);
}

void MediaRecorder::Menu() {
    ImGui::Text("录制状态：");
    ImGui::SameLine();
    switch (this->pMediaState->recordState.load()) {
    case RecordState::Free:
        ImGui::Text("空闲");
        break;
    case RecordState::Recording:
        ImGui::Text("录制编解码中");
        ImGui::Text(std::format("正在进行的录制是否是高级录制：{}",
            this->pMediaState->advancedMode ? "是" : "否").c_str());
        break;
    }
    ImGui::Text(std::format("捕获状态：{}",
        this->pMediaState->MediaSystemGlobalWorkFlag ? "正在捕获" : "无新捕获").c_str());
}

void MediaRecorder::ReportCtxState() {
    if (!this->ofctx.isOpened()) {
        this->LogInfo("输出上下文未打开");
        return;
    }
    this->LogInfo(std::format("输出上下文状态: 流数={} , 码流数={}",
        this->ofctx.streamsCount(),
        this->ofctx.streams().size()));
    for(auto&& st : this->ofctx.streams()) {
        this->LogInfo(std::format("流索引={}",
            st.index()));
    }
    this->PublishSync("MediaSync/StateReport"_hash);
    this->LogInfo("报告完毕");
}

bool MediaRecorder::Init() {
    this->pVEncodeHelper = this->FindModule<VEncodeHelper>("VEncodeHelper");
    this->pAEncodeHelper = this->FindModule<AEncodeHelper>("AEncodeHelper");
    this->pVCD3D11Manager = this->FindModule<VCD3D11Manager>("VCD3D11Manager");
    this->pMediaParamManager = this->FindModule<MediaParamManager>("MediaParamManager");
    
    (*this)
        .SubscribeAsync("Media/Record/Start")
        .SubscribeAsync("Media/Record/Stop");

    this->SendTask("Main", "AVEncoding", [this]() {
        this->Main();
        return true;
        });

    this->UIRegisterBackground("捕获以上所有根触发的UI渲染", [this](auto&&...) {
        return this->CaptureCallback();
        });

    this->UIRegisterCallback("UI.MediaSys/Control", [this](auto&&...) {
        this->Menu();
        });

    return true;
}

void MediaRecorder::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Media/Record/Start"_hash: {
        auto* ext = msg.asp.get<MulNX::NetExt>();
        this->StartRecording(ext->str1, ext->str2, this->pMediaState->nextStartUseAdvancedMode.load());
        break;
    }
    case "Media/Record/Stop"_hash:
        this->StopRecording();
        break;
    }
}

bool MediaRecorder::StartRecording(const std::string& dirPath, const std::string& fileName, bool advance) {
    if (this->pMediaState->recordState.load() != RecordState::Free) {
        this->LogWarning("已在录制中");
        return false;
    }
    this->pMediaState->advancedMode = advance;

    std::filesystem::path outputDir = std::filesystem::path(dirPath);
    std::filesystem::path outputFile = outputDir / (fileName + ".mp4");
    std::string outFile = outputFile.string();

    std::error_code ec;
    std::filesystem::create_directories(outputDir, ec);
    if (ec) {
        this->LogError(std::format("创建输出目录失败: {} ({})", outputDir.string(), ec.message()));
        return false;
    }

    this->pMediaState->pCurrentOutputDir.store(
        std::make_shared<std::filesystem::path>(outputDir),
        std::memory_order_release
    );

    int srcW = this->pGlobalVars->renderX.load(std::memory_order_acquire);
    int srcH = this->pGlobalVars->renderY.load(std::memory_order_acquire);
    av::PixelFormat srcFmt = this->pVCD3D11Manager->srcAVFormat;
    if (srcW <= 0 || srcH <= 0 || srcFmt == AV_PIX_FMT_NONE) {
        this->LogError("源纹理参数无效");
        return false;
    }

    try {
        if (this->ofctx.isOpened()) {
            this->LogWarning("输出上下文正处于打开状态！将尝试关闭");
            this->ofctx.close();
        }
    }
    catch (const std::exception& e) {
        this->LogError(std::format("在验证上下文状态时发生错误： {}", e.what()));
    }

    this->LogInfo("准备第一次Reset");
    this->PublishSync("MediaSync/Reset"_hash);
    this->LogSucc("第一次Reset完成");

    try {
        this->LogInfo(std::format("准备打开输出上下文: {}", outFile));
        this->ofctx.openOutput(outFile);
        this->LogSucc(std::format("已打开输出上下文: {}", outFile));

        MulNX::Message SetOnMsg("MediaSync/SetOn"_hash);
        auto&& [rInfo] = SetOnMsg.Access<MulNX::AVStartInfo>();
        rInfo.pOutCtx = &this->ofctx;
        rInfo.startTime = std::chrono::steady_clock::now();
        rInfo.pFilenameWithoutStem = &fileName;
        this->PublishSync(SetOnMsg);
        this->LogSucc("音视频系统SetOn完毕");

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
        this->pMediaState->recordState.store(RecordState::Recording);
        return true;
    }
    catch (const std::exception& e) {
        this->LogError(std::format("启动失败: {}", e.what()));
        this->ofctx.close();
        return false;
    }
}

bool MediaRecorder::StopRecording() {
    if (this->pMediaState->recordState.load() != RecordState::Recording) {
        this->LogWarning("未在录制中");
        return false;
    }
    this->LogInfo("准备停止录制");
    this->ReportCtxState();

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
        this->LogError(std::format("停止时异常: {}", e.what()));
    }

    this->LogInfo("冲刷完毕，准备写入尾部");
    this->ReportCtxState();

    this->ofctx.writeTrailer();
    this->LogInfo("尾部写入完毕");

    this->PublishSync("MediaSync/Reset"_hash);

    this->ofctx.close();
    this->pMediaState->pCurrentOutputDir.store(nullptr, std::memory_order_release);
    this->LogSucc("录制结束");
    this->pMediaState->recordState.store(RecordState::Free);

    return true;
}

void MediaRecorder::Main() {
    this->Update();
    try {
        this->Encode();
    }
    catch (const std::exception& e) {
        this->LogError(std::format("编码异常: {}", e.what()));
    }
}

void MediaRecorder::Encode() {
    if (this->pMediaState->recordState.load() != RecordState::Recording) return;
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