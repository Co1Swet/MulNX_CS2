#include "PlayProtect.hpp"

void PlayProtect::Menu() {
    ImGui::Text("Demo播放保护");
    MulNX::UI::Checkbox("防止Demo播放结束", this->preventEndOfMatch);
    MulNX::UI::Checkbox("强制Demo播放结束时暂停", this->forceDemoPauseAtEnd);
}

bool PlayProtect::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](auto&&...) {
        auto t = this->CS2->client.GetTextRegion().FindRegion(CS2::Signatures::Demo::CCSGO_EndOfMatch_Activate).Data();
        this->hkCCSGO_EndOfMatch_Activate = MulNX::Hook::Create(t, [this](MulNX::Hook* hk, RegContext* ctx) -> MulNX::Hook::Then {
            if (this->preventEndOfMatch.load(std::memory_order_acquire)) {
                return MulNX::Hook::Then::Return;
            }
            return MulNX::Hook::Then::Continue;
            }).value();
        this->RegisterAttachHook(this->hkCCSGO_EndOfMatch_Activate, "hkCCSGO_EndOfMatch_Activate");
        });

    this->SubscribeSync("Hook/LoadLibraryExW/engine2.dll", [this](auto&&...) {
        auto t = this->CS2->engine2.GetTextRegion().FindRegion(CS2::Signatures::Demo::Pos_CDemoPlayer_InternalReadPacket_CheckFlag_DemoPauseAtEnd).Data();
        this->hkPos_CDemoPlayer_InternalReadPacket_CheckFlag_DemoPauseAtEnd = MulNX::Hook::Create(t, [this](MulNX::Hook* hk, RegContext* ctx) -> MulNX::Hook::Then {
            if (this->forceDemoPauseAtEnd.load(std::memory_order_acquire)) {
                return MulNX::Hook::Then::SkipAllAndContinue;
            }
            return MulNX::Hook::Then::Continue;
            }, true).value();
        this->RegisterAttachHook(this->hkPos_CDemoPlayer_InternalReadPacket_CheckFlag_DemoPauseAtEnd, "hkPos_CDemoPlayer_InternalReadPacket_CheckFlag_DemoPauseAtEnd");
        });

    this->UIRegisterCallback("UI.MulNXControl", [this](auto&&...) {
        this->Menu();
        });

    return true;
}