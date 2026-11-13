#include "MediaSystem.hpp"
#include <MulNX/Base/UI/UI.hpp>

void MediaSystem::Window(MulNX::UICoordinator* uico) {
    auto w = MulNX::UI::RAIIWindow("音视频");
    if (!w || !w.ShouldDraw())return;

    uico->CallbackCall("UI.MediaSys"_hash, nullptr);
    if (ImGui::CollapsingHeader("录制控制")) {
        uico->CallbackCall("UI.MediaSys/Control"_hash, nullptr);
    }
}

bool MediaSystem::Init() {
    av::init();
    av::set_logging_level(AV_LOG_WARNING);
    this->LogSucc("FFmpeg 初始化成功");
    this->SendUIRoot(this->GetName(), [this](auto uico, auto&&...) { this->Window(uico); });
    return true;
}