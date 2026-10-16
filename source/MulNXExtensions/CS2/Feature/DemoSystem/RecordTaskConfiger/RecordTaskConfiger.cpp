#include "RecordTaskConfiger.hpp"

void RecordTaskConfiger::Window() {
    auto w = MulNX::UI::RAIIWindow("录制参数调节");
    if (!w || !w.ShouldDraw())return;
    ImGui::SliderInt("击杀前预留tick", &this->preRecordTicks, 1, 640);
    ImGui::SliderInt("击杀后保留tick", &this->postRecordTicks, 1, 640);

    ImGui::SliderInt("被击杀前预留tick", &this->preRecordTicksBekilled, 1, 640);
    ImGui::SliderInt("被击杀后保留tick", &this->postRecordTicksBekilled, 1, 640);

    MulNX::UI::SliderFloat("击杀合并阈值秒数", this->mergeThresholdSeconds, 0.0f, 200.0f);

    ImGui::SeparatorText("回合配置");
    ImGui::SliderInt("回合结束额外停留tick", &this->postTickRoundEnd, 0, 500);

    // ImGui::Checkbox("启动子弹时间（对于击杀）", &this->enableShotingTime);
    // ImGui::SliderFloat("子弹时间时间流速", &this->ShotingTimeRate, 0.01f, 1.0f);
    // ImGui::SliderInt("子弹时间前tick", &this->preTicksShotingTime, 1, 640);
    // ImGui::SliderInt("子弹时间后tick", &this->postTicksShotingTime, 1, 640);
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

int RecordTaskConfiger::GetMergeThresholdTicks()const {
    auto t = this->mergeThresholdSeconds.load(std::memory_order_acquire);
    return static_cast<int>(t * 64.0f);
}