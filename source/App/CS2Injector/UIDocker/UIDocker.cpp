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
}