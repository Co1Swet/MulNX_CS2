#include "MediaParamManager.hpp"
#include <MulNX/Base/UI/UI.hpp>

void MediaParamManager::Menu() {
    const char* modes[] = { "自动", "H264", "HEVC" };
    int mi = (int)this->mode;
    if (ImGui::Combo("编码模式", &mi, modes, IM_ARRAYSIZE(modes)))
        this->mode = (EncodeMode)mi;

    const char* rcs[] = { "CBR", "VBR", "CQ" };
    int ri = (int)this->rc;
    if (ImGui::Combo("码率控制", &ri, rcs, IM_ARRAYSIZE(rcs)))
        this->rc = (RateControl)ri;

    if (this->rc == RateControl::CQ)
        ImGui::SliderInt("CQ 质量", &this->cq, 0, 51, "%d");
    else
        ImGui::InputInt("码率(Kbps)", &this->bitrate, 1000, 10000);

    ImGui::InputInt("关键帧间隔", &this->gopSize);
    if (this->gopSize < 0) this->gopSize = 0;
    ImGui::InputInt("B 帧数", &this->maxBFrames);
    if (this->maxBFrames < 0) this->maxBFrames = 0;

    ImGui::Separator();
    ImGui::TextDisabled("画面");
    ImGui::InputInt("宽度(0=原生)", &this->width);
    ImGui::InputInt("高度(0=原生)", &this->height);
    MulNX::UI::SliderInt("捕获帧率(0=不限)", this->targetFPS, 0, 1200);
}

bool MediaParamManager::Init() {
    this->UIRegisterCallback("UI.MediaSys", [this](auto&&...) {this->Menu();});

    return true;
}