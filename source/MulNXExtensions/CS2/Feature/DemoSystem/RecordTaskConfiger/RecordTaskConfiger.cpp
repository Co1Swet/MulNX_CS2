#include "RecordTaskConfiger.hpp"

void RecordTaskConfiger::Window() {
    auto w = MulNX::UI::RAIIWindow("录制参数调节");
    if (!w || !w.ShouldDraw())return;
    MulNX::UI::SliderFloat("击杀者视角-前预留秒数", this->preRecordSeconds, 0.1f, 30.0f);
    MulNX::UI::SliderFloat("击杀者视角-后保留秒数", this->postRecordSeconds, 0.1f, 30.0f);
    MulNX::UI::SliderFloat("击杀合并阈值秒数", this->mergeThresholdSeconds, 0.0f, 200.0f);

    MulNX::UI::SliderFloat("被击杀者视角-前预留秒数", this->preBekilledSeconds, 0.1f, 30.0f);
    MulNX::UI::SliderFloat("被击杀者视角-后保留秒数", this->postBekilledSeconds, 0.1f, 30.0f);

    ImGui::SeparatorText("回合配置");
    MulNX::UI::SliderFloat("回合结束-后保留秒数", this->postRoundEndSeconds, 0, 500);
}

bool RecordTaskConfiger::Init() {

    this->UIRegisterCallback("UI.Demos", [this](auto&&...) {return this->Window();});

    this->SendTask("Update", "DemoSys", [this]()->bool {
        this->Update();
        return true;
        });

    return true;
}

void RecordTaskConfiger::ProcessMsg(MulNX::Message& msg) {
    
}