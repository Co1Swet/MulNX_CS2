#include "PlayerFlashController.hpp"
#include <MulNX/Base/UI/UI.hpp>

bool PlayerFlashController::Menu(MulNX::UINode* node) {
    MulNX::UI::Checkbox("强制移除闪光效果", this->bForceNoFlash);
    return true;
}

bool PlayerFlashController::Init() {
    this->ISys().SendUINode(this->GetName(), [this](MulNX::UINode* node) {return this->Menu(node);});
    return true;
}

void PlayerFlashController::OnItPlayer(int index, CS2::CCSPlayerController* controller, CS2::C_CSPlayerPawn* pawn) {
    if (!this->bForceNoFlash.load(std::memory_order_acquire))return;
    try {
        auto m_flFlashBangTime = MulNX::MRead(pawn->m_flFlashBangTime());
        auto m_flFlashDuration = MulNX::MRead(pawn->m_flFlashDuration());
        auto m_flFlashMaxAlpha = MulNX::MRead(pawn->m_flFlashMaxAlpha());
        auto m_flFlashOverlayAlpha = MulNX::MRead(pawn->m_flFlashOverlayAlpha());
        auto m_flFlashScreenshotAlpha = MulNX::MRead(pawn->m_flFlashScreenshotAlpha());
        auto m_bFlashBuildUp = MulNX::MRead(pawn->m_bFlashBuildUp());
        auto m_bFlashDspHasBeenCleared = MulNX::MRead(pawn->m_bFlashDspHasBeenCleared());
        auto m_bFlashScreenshotHasBeenGrabbed = MulNX::MRead(pawn->m_bFlashScreenshotHasBeenGrabbed());

        MulNX::MWrite(pawn->m_flFlashBangTime(), 0.0f);
        MulNX::MWrite(pawn->m_flFlashDuration(), 0.0f);
        MulNX::MWrite(pawn->m_flFlashMaxAlpha(), 0.0f);
        MulNX::MWrite(pawn->m_flFlashOverlayAlpha(), 0.0f);
        MulNX::MWrite(pawn->m_flFlashScreenshotAlpha(), 0.0f);
    }
    catch (const std::runtime_error& e) {
        this->ISys().LogWarning(std::format("在修改闪光弹效果时发生错误：{}", e.what()).c_str());
    }

    return;
}