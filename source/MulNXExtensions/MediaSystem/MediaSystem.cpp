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

    return true;
}

bool MediaSystem::Init() {
    av::init();
    av::set_logging_level(AV_LOG_WARNING);

    this->ISys().LogSucc("FFmpeg 与 AvCpp 初始化成功！");

    this->ISys().SendUINode(this->GetName(), [this](MulNX::UINode* node) {return this->Window(node);});

    return true;
}