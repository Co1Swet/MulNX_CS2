#include "RecordTaskConfiger.hpp"
#include <MulNX/Base/UI/UI.hpp>

bool RecordTaskConfiger::Window(MulNX::UINode* node) {
    auto w = MulNX::UI::RAIIWindow("录制参数调节");
    if (!w)return true;
    ImGui::SliderInt("击杀前预留tick", &this->preRecordTicks, 1, 640);
    ImGui::SliderInt("击杀后保留tick", &this->postRecordTicks, 1, 640);

    ImGui::SliderInt("被击杀前预留tick", &this->preRecordTicksBekilled, 1, 640);
    ImGui::SliderInt("被击杀后保留tick", &this->postRecordTicksBekilled, 1, 640);

    // ImGui::Checkbox("启动子弹时间（对于击杀）", &this->enableShotingTime);
    // ImGui::SliderFloat("子弹时间时间流速", &this->ShotingTimeRate, 0.01f, 1.0f);
    // ImGui::SliderInt("子弹时间前tick", &this->preTicksShotingTime, 1, 640);
    // ImGui::SliderInt("子弹时间后tick", &this->postTicksShotingTime, 1, 640);
    return true;
}

bool RecordTaskConfiger::Init() {

    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {return this->Window(node);});

    this->SendTask("DemoSys", [this]()->bool {
        this->Update();
        return true;
        });

    this->showWindow.store(true, std::memory_order_release);

    return true;
}

void RecordTaskConfiger::ProcessMsg(MulNX::Message& msg) {
    
}