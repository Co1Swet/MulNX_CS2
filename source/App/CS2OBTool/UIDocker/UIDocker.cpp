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
    uico->CallUINode("GraphicsManager");
    uico->CallUINode("PlayerFlashController");
    uico->CallUINode("ESPBox");
    uico->CallUINode("ESPSkeleton");
    uico->CallUINode("GameHudMenu");
    uico->CallUINode("BombSpotController");
    uico->CallUINode("PlayerSpotRenderController");
    uico->CallUINode("PlayerSpotColorController");
    uico->CallUINode("TeamCounterController");
    uico->CallUINode("FlashRenderController");
    MulNX::UI::Checkbox(I18n("dthmsg.window.control").c_str(), this->Core->ModuleManager()->FindModule("DeathMsgController")->showWindow);
    ImGui::End();

    ImGui::Begin(I18n("高级功能").c_str());
    //MulNX::UI::Checkbox("小地图窗口", this->Core->ModuleManager()->FindModule("MiniMap")->showWindow);
    //MulNX::UI::Checkbox("游戏配置管理器窗口", this->Core->ModuleManager()->FindModule("GameCfgManager")->showWindow);
    MulNX::UI::Checkbox("Demo", this->Core->ModuleManager()->FindModule("DemoSystem")->showWindow);
    MulNX::UI::Checkbox("玩家信息管理窗口", this->Core->ModuleManager()->FindModule("PlayerHub")->showWindow);
    ImGui::End();

    ImGui::Begin(I18n("3D视觉").c_str());
    uico->CallUINode("SkinController");
    uico->CallUINode("TeamIDRenderController");
    uico->CallUINode("TrailsController");
    uico->CallUINode("SkyController");
    ImGui::End();

    ImGui::Begin(I18n("镜头参数").c_str());
    uico->CallUINode("HookView");
    uico->CallUINode("FreeCameraController");
    uico->CallUINode("DofMenu");
    ImGui::End();

    ImGui::Begin(I18n("视角视图").c_str());
    uico->CallUINode("ProjectileTracker");
    uico->CallUINode("AdvancedViewController");
    ImGui::End();

    ImGui::Begin(I18n("ui.camera_system").c_str());
    uico->CallUINode("CameraSystem");
    ImGui::End();

    ImGui::Begin(I18n("ui.game_settings").c_str());
    uico->CallUINode("GameSettingsManager");
    ImGui::End();

    ImGui::Begin(I18n("ui.mulnx_control").c_str());
    uico->CallUINode("VirtualUser");
    uico->CallUINode("MulNXController");
    ImGui::End();
}