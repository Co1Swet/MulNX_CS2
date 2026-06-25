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

    ImGui::Begin(I18n("ui.mulnx_control").c_str());
    node->CallUINode("MulNXController");
    ImGui::End();

    node->CallUINode("UIDocker");
    node->CallUINode("Debugger");
    node->CallUINode("CS2BootLoader");
}