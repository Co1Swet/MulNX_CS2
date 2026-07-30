#include "MediaSystem.hpp"
#include <MulNXExtensions/MediaSystem/MediaRecorder/MediaRecorder.hpp>
#include <MulNXExtensions/MediaSystem/MediaParamManager/MediaParamManager.hpp>
#include <MulNX/Base/UI/UI.hpp>

void MediaSystem::Window(MulNX::UICoordinator* uico) {
    auto w = MulNX::UI::RAIIWindow("音视频");

    uico->CallbackCall("UI.MediaSys"_hash, nullptr);

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
}

bool MediaSystem::Init() {
    av::init();
    av::set_logging_level(AV_LOG_WARNING);

    this->dirVideos = this->Path()->PathGetForShared("Videos");
    this->LogSucc("FFmpeg 初始化成功");

    this->SendUIRoot(this->GetName(), [this](auto uico, auto&&...) { this->Window(uico); });
    
    return true;
}