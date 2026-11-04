#include "CS2Test.hpp"
#include <MulNXUtils/AddressCheck.hpp>
#include <Feature/HUD/HookDamageReport/HookDamageReport.hpp>
#include <unordered_set>
#include <Game/StaticWeapons/StaticWeapons.hpp>

void CS2Test::UI() {
    auto pLocalPawn = this->CS2Entitys->GetLocalPlayerPawnEx();
    if (!pLocalPawn)return;
    auto observerService = MulNX::MRead(pLocalPawn->pObserverServices());
    auto pMode = observerService->iObserverMode();
    auto pLocalController = this->CS2->client.dwLocalPlayerController();
    auto pDamageServices = MulNX::MRead(pLocalController->m_pDamageServices());
    auto pSize = &pDamageServices->m_DamageList.m_nSize;

    uint64_t address = (uint64_t)pSize;
    char buf[32];
    snprintf(buf, sizeof(buf), "0x%016llX", address);

    ImGui::InputText("Address", buf, sizeof(buf),
        ImGuiInputTextFlags_ReadOnly);
}

bool CS2Test::Init() {
    std::thread([]() {
        MessageBoxW(NULL, L"MulNX 注入成功！", L"MulNX", MB_OK | MB_ICONINFORMATION);
        }).detach();

    //this->AsyncCommand("playdemo 111");
    this->AsyncCommand("tv_listen_voice_indices -1");

    this->SendUIRoot("MyCS2Test", [this](auto&&...) {
        try {
            this->UI();
        }
        catch (MulNX::Exception& e) {

        }
        });

    auto k1 = CS2::FindWeapon("weapon_ak47")->damageType;

    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](auto&&...) {

        auto t = this->CS2->client.GetTextRegion().FindRegion(CS2::Signatures::Demo::CCSGO_EndOfMatch_Activate).Data();
        static auto hookShow = MulNX::Hook::Create(t, [this](MulNX::Hook* hk, RegContext* ctx) -> MulNX::Hook::Then {
            return MulNX::Hook::Then::Return;   // 整个函数不执行
            }).value();
        hookShow->Attach();
        });

    this->SubscribeSync("Hook/LoadLibraryExW/engine2.dll", [this](auto&&...) {
        auto t2 = this->CS2->engine2.GetTextRegion().FindRegion(CS2::Signatures::Demo::Pos_CDemoPlayer_InternalReadPacket_CheckFlag_DemoPauseAtEnd).Data();
        static auto hookShow2 = MulNX::Hook::Create(t2, [this](MulNX::Hook* hk, RegContext* ctx) -> MulNX::Hook::Then {
            return MulNX::Hook::Then::SkipAllAndContinue;
            }, true).value();
        hookShow2->Attach();
        });

    return true;
}