#include "CloudController.hpp"

void CloudController::Menu() {
    MulNX::UI::Checkbox("隐藏云层", this->hideCloud);
    MulNX::UI::Checkbox("隐藏太阳", this->hideSun);
}

bool CloudController::Init() {

    this->UIRegisterCallback("UI.3DVision", [this](auto&&...) { this->Menu(); });

    return true;
}

bool CloudController::NeedHide(const char* matName) {
    if (strstr(matName, "clouds") && this->hideCloud.load(std::memory_order_acquire)) {
        return true;
    }
    else if (strstr(matName, "sun_disc_glow") && this->hideSun.load(std::memory_order_acquire)) {
        return true;
    }
    return false;
}