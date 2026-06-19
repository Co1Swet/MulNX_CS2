#include "UIDocker.hpp"
#include <MulNX/Base/UI/UI.hpp>

bool UIDocker::Init() {
    this->SendUIRoot("MainDraw", [this](MulNX::UINode* node) {this->MainDraw(node);});
    return true;
}

void UIDocker::MainDraw(MulNX::UINode* node) {
    ImGui::Begin(I18n("ui.main").c_str());
    ImGui::Text(I18n("homepage").c_str());
    ImGui::End();

    ImGui::Begin(I18n("声音控制").c_str());
    node->CallUINode("SoundMenu");
    ImGui::End();

    ImGui::Begin(I18n("2D视觉").c_str());
    node->CallUINode("GraphicsManager");
    node->CallUINode("PlayerFlashController");
    MulNX::UI::Checkbox("ESP", this->Core->ModuleManager()->FindModule("ESPController")->showWindow);
    node->CallUINode("GameHudMenu");
    ImGui::End();

    ImGui::Begin(I18n("高级功能").c_str());
    MulNX::UI::Checkbox("小地图窗口", this->Core->ModuleManager()->FindModule("MiniMap")->showWindow);
    MulNX::UI::Checkbox("游戏配置管理器窗口", this->Core->ModuleManager()->FindModule("GameCfgManager")->showWindow);
    MulNX::UI::Checkbox("Demo", this->Core->ModuleManager()->FindModule("DemoSystem")->showWindow);
    MulNX::UI::Checkbox("玩家信息管理窗口", this->Core->ModuleManager()->FindModule("PlayerHub")->showWindow);
    MulNX::UI::Checkbox(I18n("dthmsg.window.control").c_str(), this->Core->ModuleManager()->FindModule("DeathMsgController")->showWindow);
    ImGui::End();

    ImGui::Begin(I18n("3D视觉").c_str());
    node->CallUINode("SkinController");
    ImGui::End();

    ImGui::Begin(I18n("镜头参数").c_str());
    node->CallUINode("ViewController");
    node->CallUINode("FreeCameraController");
    node->CallUINode("DofMenu");
    ImGui::End();

    ImGui::Begin(I18n("视角视图").c_str());
    node->CallUINode("AdvancedViewController");
    ImGui::End();

    ImGui::Begin(I18n("ui.camera_system").c_str());
    node->CallUINode("CameraSystem");
    ImGui::End();

    ImGui::Begin(I18n("ui.game_settings").c_str());
    node->CallUINode("GameSettingsManager");
    ImGui::End();

    ImGui::Begin(I18n("ui.mulnx_control").c_str());
    node->CallUINode("VirtualUser");
    node->CallUINode("MulNXController");
    ImGui::End();

    node->CallUINode("UIDocker");
    node->CallUINode("Debugger");
    node->CallUINode("GameCfgManager");
    node->CallUINode("MiniMap");
    node->CallUINode("CSController");
    node->CallUINode("ElementManager");
    node->CallUINode("SolutionManager");
    node->CallUINode("ProjectManager");
    node->CallUINode("DemoSystem");
    node->CallUINode("PlayerHub");
    node->CallUINode("ProjectileTracker");
    node->CallUINode("DeathMsgController");
    node->CallUINode("ESPController");
    node->CallUINode("MediaSystem");
    node->CallUINode("EntityListScanner");
}