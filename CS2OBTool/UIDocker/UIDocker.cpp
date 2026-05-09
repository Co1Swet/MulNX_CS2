#include "UIDocker.hpp"
#include <MulNX/Base/UI/UI.hpp>

bool UIDocker::Init() {
    this->SendUINode("MainDraw", [this](MulNX::UINode* node) {this->MainDraw(node);});
    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {this->Window(node);});
    return true;
}

void UIDocker::Window(MulNX::UINode* node) {
    this->ShowWindow = true;
    auto w = MulNX::UI::RAIIWindow(I18n("ui.settings").c_str(), this->ShowWindow);

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 fullSize = viewport->Size;

    ImGui::SliderFloat(I18n("ui.padding.top").c_str(), &this->padding.top, 0.0f, fullSize.y / 2);
    ImGui::SliderFloat(I18n("ui.padding.bottom").c_str(), &this->padding.bottom, 0.0f, fullSize.y / 2);
    ImGui::SliderFloat(I18n("ui.padding.left").c_str(), &this->padding.left, 0.0f, fullSize.x / 2);
    ImGui::SliderFloat(I18n("ui.padding.right").c_str(), &this->padding.right, 0.0f, fullSize.x / 2);

    ImGui::Separator();
    node->CallUINode("UISystem");
}

void UIDocker::MainDraw(MulNX::UINode* node) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 fullSize = viewport->Size;

    this->padding.top = std::clamp(this->padding.top, 0.0f, fullSize.y / 2);
    this->padding.bottom = std::clamp(this->padding.bottom, 0.0f, fullSize.y / 2);
    this->padding.left = std::clamp(this->padding.left, 0.0f, fullSize.x / 2);
    this->padding.right = std::clamp(this->padding.right, 0.0f, fullSize.x / 2);

    // auto clampSum = [](float& a, float& b, float maxSum) {
    //     float sum = a + b;
    //     if (sum > maxSum) {
    //         float scale = maxSum / sum;
    //         a *= scale;
    //         b *= scale;
    //     }
    // };
    // clampSum(this->padding.left, this->padding.right, fullSize.x - 1.0f);
    // clampSum(this->padding.top, this->padding.bottom, fullSize.y - 1.0f);

    ImVec2 dockPos(
        viewport->Pos.x + this->padding.left,
        viewport->Pos.y + this->padding.top
    );
    ImVec2 dockSize(
        fullSize.x - this->padding.left - this->padding.right,
        fullSize.y - this->padding.top - this->padding.bottom
    );

    ImGui::SetNextWindowPos(dockPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(dockSize, ImGuiCond_Always);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, IM_COL32(0, 0, 0, 0));

    ImGui::Begin("DockRoot", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoBackground);

    ImGui::DockSpace(ImGui::GetID("MainDockSpace"), ImVec2(0, 0),
        ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::End();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);

    ImGui::Begin(I18n("ui.main").c_str());
    ImGui::Text(I18n("homepage").c_str());
    ImGui::End();

    ImGui::Begin(I18n("ui.camera_system").c_str());
    node->CallUINode("CameraSystem");
    ImGui::End();

    ImGui::Begin(I18n("ui.game_settings").c_str());
    node->CallUINode("GameSettingsManager");
    ImGui::End();

    ImGui::Begin(I18n("ui.game_enhance").c_str());
    node->CallUINode("POVFixer");
    node->CallUINode("ConsoleManager");
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
    node->CallUINode("MediaRemoter");
    node->CallUINode("ESPController");
}