#include "MediaSystem.hpp"
#include <MulNX/Base/UI/UI.hpp>

void MediaSystem::Window(MulNX::UICoordinator* uico) {
    auto w = MulNX::UI::RAIIWindow("音视频");
    if (!w || !w.ShouldDraw())return;

    uico->CallbackCall("UI.MediaSys"_hash, nullptr);
    if (ImGui::CollapsingHeader("录制控制")) {
        uico->CallbackCall("UI.MediaSys/Control"_hash, nullptr);
    }
    if (ImGui::Button("测试")) {
        auto pv = this->Path()->PathGetForShared("Videos");
        auto [beginMsg, beginExt] = MulNX::Message::Create<MulNX::NetExt>("Media/Concat/Begin"_hash);
        beginExt->str1 = (pv / "testend.mp4").string();
        this->PublishAsync(std::move(beginMsg));

        for (const auto& name : { "test1.mp4", "test2.mp4", "test3.mp4", "test4.mp4" }) {
            auto [addMsg, addExt] = MulNX::Message::Create<MulNX::NetExt>("Media/Concat/Add"_hash);
            addExt->str1 = (pv / name).string();
            this->PublishAsync(std::move(addMsg));
        }

        auto [endMsg, endExt] = MulNX::Message::Create<MulNX::NetExt>("Media/Concat/End"_hash);
        this->PublishAsync(std::move(endMsg));
    }
}

bool MediaSystem::Init() {
    av::init();
    av::set_logging_level(AV_LOG_WARNING);
    this->LogSucc("FFmpeg 初始化成功");
    this->SendUIRoot(this->GetName(), [this](auto uico, auto&&...) { this->Window(uico); });
    return true;
}