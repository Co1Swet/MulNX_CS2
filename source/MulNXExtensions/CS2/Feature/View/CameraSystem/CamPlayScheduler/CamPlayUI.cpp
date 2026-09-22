#include "CamPlayScheduler.hpp"

void CamPlayScheduler::DrawPlayCamera() {
    if (!this->drawCam.load(std::memory_order_acquire))return;
    if (!this->needDrawCamera.load(std::memory_order_acquire))return;
    
    auto frame = this->drawCamera.Read();
    this->pCamDrawer->DrawFrameCamera(*frame, "当前播放摄像机");
}

void CamPlayScheduler::Menu() {

    ImGui::Text("播放控制");
    auto bDraw = this->drawCam.load(std::memory_order_acquire);
    if (ImGui::Checkbox("绘制播放摄像机",&bDraw)) {
        if (bDraw) {
            this->PublishAsync("CamPlay/Draw/Enable"_hash);
        }
        else {
            this->PublishAsync("CamPlay/Draw/Disable"_hash);
        }
    }

    auto bOverride = this->camOverride.load(std::memory_order_acquire);
    if (ImGui::Checkbox("播放覆盖游戏视角", &bOverride)) {
        if (bOverride) {
            this->PublishAsync("CamPlay/Override/Enable"_hash);
        }
        else {
            this->PublishAsync("CamPlay/Override/Disable"_hash);
        }
    }
    ImGui::Text("alt+P以 停止所有预览（包括宏触发），alt+P*2清理所有运镜");
}