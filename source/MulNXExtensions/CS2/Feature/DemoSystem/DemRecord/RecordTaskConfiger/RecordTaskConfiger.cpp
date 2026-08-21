#include "RecordTaskConfiger.hpp"
#include <MulNX/Base/UI/UI.hpp>

void RecordTaskConfiger::Window() {
    auto w = MulNX::UI::RAIIWindow("录制参数调节");
    if (!w || !w.ShouldDraw())return;
    ImGui::SliderInt("击杀前预留tick", &this->preRecordTicks, 1, 640);
    ImGui::SliderInt("击杀后保留tick", &this->postRecordTicks, 1, 640);

    ImGui::SliderInt("被击杀前预留tick", &this->preRecordTicksBekilled, 1, 640);
    ImGui::SliderInt("被击杀后保留tick", &this->postRecordTicksBekilled, 1, 640);

    ImGui::SliderFloat("合并阈值tick", &this->mergeThresholdTicks, 0.0f, 1000.0f);

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