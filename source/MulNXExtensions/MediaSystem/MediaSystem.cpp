#include "MediaSystem.hpp"
#include <MulNXExtensions/MediaSystem/MediaRecorder/MediaRecorder.hpp>
#include <MulNXExtensions/MediaSystem/MediaParamManager/MediaParamManager.hpp>
#include <MulNX/Base/UI/UI.hpp>

bool MediaSystem::Window(MulNX::UINode* node) {
    auto w = MulNX::UI::RAIIWindow("音视频");
    this->RecordParamsUI();

    ImGui::Separator();
    ImGui::InputText("文件名", &this->outputFile);

    if (ImGui::Button("开始录制")) {
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Media/Record/Start"_hash);
        rp->str1 = (this->dirVideos / this->outputFile).string();
        this->PublishAsync(std::move(msg));
    }
    ImGui::SameLine();
    if (ImGui::Button("结束录制")) {
        this->PublishAsync("Media/Record/Stop"_hash);
    }

    // if (ImGui::Button("测试")) {
    //     auto pv = this->Path()->PathGetForShared("Videos");
    //     auto s = [&](auto m) {
    //         auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>(m);
    //         rp->str1 = (pv / "testend.mp4").string();
    //         this->PublishAsync(std::move(msg));
    //     };
    //     s("Media/Concat/Begin"_hash);
    //     for (const auto& n : { "test1.mp4", "test2.mp4" }) {
    //         auto [m, r] = MulNX::Message::Create<MulNX::NetExt>("Media/Concat/Add"_hash);
    //         r->str1 = (pv / n).string(); this->PublishAsync(std::move(m));
    //     }
    //     s("Media/Concat/End"_hash);
    // }
    return true;
}

void MediaSystem::RecordParamsUI() {
    if (!this->pMediaParamManager) return;
    RecordParams& p = this->pMediaParamManager->Params();
    const EncoderCaps& caps = this->pMediaParamManager->Caps();

    ImGui::TextDisabled("编码器");
    ImGui::Text("硬编:%d  软编:%d", (int)caps.hwEncoders.size(), (int)caps.swEncoders.size());

    const char* modes[] = { "自动", "H264", "HEVC" };
    int mi = (int)p.mode;
    if (ImGui::Combo("编码模式", &mi, modes, IM_ARRAYSIZE(modes))) p.mode = (EncodeMode)mi;

    const char* rcs[] = { "CBR", "VBR", "CQ" };
    int ri = (int)p.rc;
    if (ImGui::Combo("码率控制", &ri, rcs, IM_ARRAYSIZE(rcs))) p.rc = (RateControl)ri;

    if (p.rc == RateControl::CQ)
        ImGui::SliderInt("CQ 质量", &p.cq, 0, 51, "%d");
    else
        ImGui::InputInt("码率(Kbps)", &p.bitrate, 1000, 10000);

    ImGui::InputInt("关键帧间隔", &p.gopSize);
    if (p.gopSize < 0) p.gopSize = 0;
    ImGui::InputInt("B 帧数", &p.maxBFrames);
    if (p.maxBFrames < 0) p.maxBFrames = 0;

    ImGui::Separator();
    ImGui::TextDisabled("画面");
    ImGui::InputInt("宽度(0=原生)", &p.width);
    ImGui::InputInt("高度(0=原生)", &p.height);
    ImGui::InputInt("捕获帧率(0=不限)", &p.captureFpsCap);
    if (p.captureFpsCap < 0) p.captureFpsCap = 0;
}

bool MediaSystem::Init() {
    av::init();
    av::set_logging_level(AV_LOG_WARNING);
    this->pMediaParamManager = this->Core->ModuleManager()->FindModule<MediaParamManager>("MediaParamManager");
    this->LogSucc("FFmpeg 初始化成功");
    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) { return this->Window(node); });
    this->dirVideos = this->Path()->PathGetForShared("Videos");

    return true;
}
