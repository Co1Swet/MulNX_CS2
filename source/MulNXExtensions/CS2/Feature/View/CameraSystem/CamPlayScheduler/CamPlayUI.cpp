#include "CamPlayScheduler.hpp"

void CamPlayScheduler::Menu() {

    ImGui::SeparatorText("预览总控");
    //ImGui::Text(std::format("是否允许预览摄像机绘制：{}", this->Config.PreviewDraw ? "允许" : "不允许").c_str());
    if (ImGui::Button("启用预览摄像机绘制")) {
        this->PublishAsync("Campath/Preview/Draw/Enable"_hash);
    }
    ImGui::SameLine();
    if (ImGui::Button("禁用预览摄像机绘制")) {
        this->PublishAsync("Campath/Preview/Draw/Disable"_hash);
    }
    // ImGui::Text(std::format("是否允许预览摄像机覆盖游戏摄像机：{}",
    //     this->Config.PreviewOverride ? "允许" : "不允许").c_str());
    if (ImGui::Button("启用预览摄像机覆盖")) {
        this->PublishAsync("Campath/Preview/Override/Enable"_hash);
    }
    ImGui::SameLine();
    if (ImGui::Button("禁用预览摄像机覆盖")) {
        this->PublishAsync("Campath/Preview/Override/Disable"_hash);
    }
}