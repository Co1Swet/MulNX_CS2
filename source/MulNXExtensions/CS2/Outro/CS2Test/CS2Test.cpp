#include "CS2Test.hpp"
#include <MulNXUtils/AddressCheck.hpp>
#include <Feature/HUD/HookDamageReport/HookDamageReport.hpp>
#include <unordered_set>
#include <Game/StaticWeapons/StaticWeapons.hpp>
#include <Support/ClientPanorama/ClientPanorama.hpp>

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

    (*this)
        .SubscribeAsync("Core/Tick30min");

    //this->AsyncCommand("playdemo 111");
    this->AsyncCommand("tv_listen_voice_indices -1");

    this->SendUIRoot("MyCS2Test", [this](auto&&...) {
        try {
            this->Update();
            this->UI();

            
        }
        catch (MulNX::Exception& e) {

        }
        });

    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](auto&&...) {
        auto t = this->CS2->client.GetBaseAddress() + 0xEC0DC0;
        static auto hook = MulNX::Hook::Create((uint8_t*)t, [this](MulNX::Hook* hk, RegContext* ctx) {
            const char* p = (const char*)(ctx->rdx);
            p += 3;
            auto s = MulNX::Memory::ReadString(p);
            return MulNX::Hook::Then::Return;
            }) | value_or_throw;
        hook->Attach();
        });
    
    this->SubscribeSync("Hook/CSMainLoop", [&](auto&&...) {
        auto t = this->CS2->client.GetBaseAddress() + 0xE7AF90;
        if (this->pInputSystem->IsKeyPressed('K')) {
            auto a1 = this->FindModule<ClientPanorama>()->FindHudElement("CCSGO_HudChat") - 32;
            using f = __int64 (*)(__int64 a1, const char* a2);
            reinterpret_cast<f>(t)(a1, "test");
        }
        });

    auto k1 = CS2::FindWeapon("weapon_ak47")->damageType;

    return true;
}

void CS2Test::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Core/Tick30min"_hash: {
        //this->AsyncCommand("playdemo 111");
        break;
    }
    }
}