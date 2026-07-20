#include "UIDocker.hpp"
#include <MulNX/Base/UI/UI.hpp>

bool UIDocker::Init() {
    this->SendUIRoot("MainDraw", [this](auto uico, auto&&...) {this->MainDraw(uico);});
    return true;
}

void UIDocker::MainDraw(MulNX::UICoordinator* uico) {
    ImGui::Begin(I18n("ui.main").c_str());
    ImGui::Text(I18n("homepage").c_str());
    ImGui::End();

    ImGui::Begin(I18n("声音控制").c_str());
    uico->CallUINode("SoundMenu");
    uico->CallUINode("SpeakingController");
    ImGui::End();

    ImGui::Begin(I18n("2D视觉").c_str());
    uico->CallbackCall("UI.2DVision"_hash, nullptr);
    MulNX::UI::Checkbox(I18n("dthmsg.window.control").c_str(), this->Core->ModuleManager()->FindModule("DeathMsgController")->showWindow);
    ImGui::End();

    ImGui::Begin(I18n("高级功能").c_str());
    uico->CallbackCall("UI.Advanced"_hash, nullptr);
    ImGui::End();

    ImGui::Begin(I18n("3D视觉").c_str());
    uico->CallbackCall("UI.3DVision"_hash, nullptr);
    ImGui::End();

    ImGui::Begin(I18n("镜头参数").c_str());
    uico->CallUINode("HookView");
    uico->CallUINode("FreeCameraController");
    uico->CallUINode("DofMenu");
    ImGui::End();

    ImGui::Begin(I18n("视角视图").c_str());
    uico->CallbackCall("UI.View"_hash, nullptr);
    ImGui::End();

    ImGui::Begin(I18n("ui.game_settings").c_str());
    uico->CallUINode("GameSettingsManager");
    ImGui::End();

    ImGui::Begin(I18n("ui.mulnx_control").c_str());
    uico->CallUINode("VirtualUser");
    uico->CallUINode("MulNXController");
    ImGui::End();
}