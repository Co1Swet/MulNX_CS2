#include "MediaParamManager.hpp"
#include <MulNX/Base/UI/UI.hpp>

void MediaParamManager::Menu() {
    if (ImGui::CollapsingHeader("全局参数")) {
        MulNX::UI::SliderInt("宽度(0=原生)", this->width, 0, 20000);
        MulNX::UI::SliderInt("高度(0=原生)", this->height, 0, 10000);
        MulNX::UI::SliderInt("捕获帧率", this->targetFPS, 0, 10000);

        const char* rcs[] = { "CBR", "VBR", "CQ" };
        int ri = (int)this->rc;
        if (ImGui::Combo("码率控制", &ri, rcs, IM_ARRAYSIZE(rcs)))
            this->rc = (RateControl)ri;

        if (this->rc == RateControl::CQ)
            MulNX::UI::SliderInt("CQ 质量", this->cq, 0, 51);
        else
            MulNX::UI::SliderInt("码率(bps)", this->bitrate, 1000, INT_MAX / 2);

        MulNX::UI::SliderInt("关键帧间隔", this->gopSize, 0, 120);
        if (this->gopSize < 0) this->gopSize = 0;
        MulNX::UI::SliderInt("B 帧数", this->maxBFrames, 0, 10);
        if (this->maxBFrames < 0) this->maxBFrames = 0;
    }
}

bool MediaParamManager::Init() {
    this->UIRegisterCallback("UI.MediaSys", [this](auto&&...) {this->Menu();});

    return true;
}