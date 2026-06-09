#include "MediaSystem.hpp"
#include <MulNXExtensions/MediaSystem/MediaRecorder/MediaRecorder.hpp>
#include <MulNX/Base/UI/UI.hpp>

bool MediaSystem::Window(MulNX::UINode* node) {
    auto w = MulNX::UI::RAIIWindow("音视频");

    if (ImGui::Button("开始录制")) {
        this->ISys().PublishAsync("Media/Record/Start"_hash);
    }
    if (ImGui::Button("结束录制")) {
        this->ISys().PublishAsync("Media/Record/Stop"_hash);
    }

    if (ImGui::Button("测试")) {
        auto pathVedios = this->ISys().PathManager()->PathGetForShared("Vedios");

        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Media/Concat/Begin"_hash);
        rp->str1 = (pathVedios / "testend.mp4").string();
        this->ISys().PublishAsync(std::move(msg));

        auto [msg2, rp2] = MulNX::Message::Create<MulNX::NetExt>("Media/Concat/Add"_hash);
        rp2->str1 = (pathVedios / "test1.mp4").string();
        this->ISys().PublishAsync(std::move(msg2));

        auto [msg3, rp3] = MulNX::Message::Create<MulNX::NetExt>("Media/Concat/Add"_hash);
        rp3->str1 = (pathVedios / "test2.mp4").string();
        this->ISys().PublishAsync(std::move(msg3));

        auto [msg4, rp4] = MulNX::Message::Create<MulNX::NetExt>("Media/Concat/End"_hash);
        this->ISys().PublishAsync(std::move(msg4));
    }

    return true;
}

bool MediaSystem::Init() {
    av::init();
    av::set_logging_level(AV_LOG_WARNING);

    this->ISys().LogSucc("FFmpeg 与 AvCpp 初始化成功！");

    this->ISys().SendUINode(this->GetName(), [this](MulNX::UINode* node) {return this->Window(node);});

    return true;
}