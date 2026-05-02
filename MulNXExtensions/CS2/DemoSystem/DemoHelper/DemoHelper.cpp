#include "DemoHelper.hpp"

#include <MulNX/MulNX.hpp>
#include <MulNX/Base/UI/UI.hpp>

bool DemoHelper::UINodeFunc(MulNX::UINode* node) {
    auto w = MulNX::UI::RAIIWindow("Demo辅助", this->ShowWindow);
    if (!w)return true;
    std::shared_lock lock(this->smutex);
    if (ImGui::Button("标记当前时间")) {
        MulNX::Message msg("DemoHelper/MarkTime"_hash);
        node->PublishAsync(std::move(msg));
    }
    ImGui::Text("时间列表:");
    for (auto time : this->TimeMarks) {
        ImGui::Text("时间点： %.3f 秒", time);
        ImGui::SameLine();
        std::string str = "跳转##" + std::to_string(time);
        if (ImGui::Button(str.c_str())) {
            MulNX::Message Msg("DemoHelper/JumpTIme"_hash);
            Msg.p1.low<float>() = time;
            node->PublishAsync(std::move(Msg));
        }
    }
    ImGui::SeparatorText("快捷时间");
    if (ImGui::Button("五秒前")) {
        this->AL3D->Time()->JumpRealRel(-5.0f);
    }
    ImGui::SameLine();
    if (ImGui::Button("一秒前")) {
        this->AL3D->Time()->JumpRealRel(-1.0f);
    }
    ImGui::SameLine();
    if (ImGui::Button("一秒后")) {
        this->AL3D->Time()->JumpRealRel(1.0f);
    }
    ImGui::SameLine();
    if (ImGui::Button("五秒后")) {
        this->AL3D->Time()->JumpRealRel(5.0f);
    }
    ImGui::Separator();
    static float delta = 0.5f;
    ImGui::SliderFloat("自定义时间差", &delta, 0.0f, 60.0f);
    if (ImGui::Button("前跳")) {
        this->AL3D->Time()->JumpRealRel(-delta);
    }
    ImGui::SameLine();
    if (ImGui::Button("后跳")) {
        this->AL3D->Time()->JumpRealRel(delta);
    }

    return true;
}

bool DemoHelper::Init() {
    this->ISys()
        .SubscribeAsync("DemoHelper/MarkTime")
        .SubscribeAsync("DemoHelper/JumpTIme")
        ;

    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {return this->UINodeFunc(node);});
    this->SendTask("MulNXMain", [this]()->bool {
        this->Main();
        return true;
        });
    return true;
}

void DemoHelper::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "DemoHelper/MarkTime"_hash: {
        this->MarkTime();
        break;
    }
    case "DemoHelper/JumpTIme"_hash: {
        float data = msg.p1.low<float>();
        std::string str = "跳转到" + std::to_string(data);
        this->ISys().LogInfo(str);
        this->AL3D->Time()->JumpReal(data);
        break;
    }
    }
}

void DemoHelper::Main() {
    this->EntryProcessMsg();
}

bool DemoHelper::MarkTime() {
    std::unique_lock lock(this->smutex);
    this->Marks.push_back(this->AL3D->Time()->GetReal());

    return true;
}