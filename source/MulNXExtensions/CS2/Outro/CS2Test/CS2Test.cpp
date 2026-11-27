#include "CS2Test.hpp"
#include <MulNXUtils/AddressCheck.hpp>
#include <Feature/HUD/HookDamageReport/HookDamageReport.hpp>
#include <unordered_set>
#include <Game/StaticWeapons/StaticWeapons.hpp>
#include <Support/ClientPanorama/ClientPanorama.hpp>
#include <Mirror/ParticleManager/ParticleManager.hpp>
#include <Mirror/CS2Hash/CS2Hash.hpp>

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

    this->AsyncCommand("playdemo 111");
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
        //hook->Attach();

        auto t2 = this->CS2->client.GetBaseAddress() + 0xBE47D0;
        static auto hook2 = MulNX::Hook::Create((uint8_t*)t2, [this](MulNX::Hook* hk, RegContext* ctx) {
            const char* p = (const char*)(ctx->rcx);
            auto s = MulNX::Memory::ReadString(p).value();
            this->LogInfo(std::string(s));
            return MulNX::Hook::Then::Continue;
            }) | value_or_throw;
        hook2->Attach();
        
        });

    this->SubscribeSync("Hook/LoadLibraryExW/engine2.dll", [this](auto&&...) {
        auto engine2base = (uint8_t*)this->CS2->engine2.GetBaseAddress();

        static auto hook2 = MulNX::Hook::Create(engine2base + 0x1ACA70, [this](MulNX::Hook* hk, RegContext* ctx) {
            const char* p = (const char*)(ctx->rdx);
            auto s = MulNX::Memory::ReadString(p).value();
            if (s.find("rain_fx") != std::string::npos) {
                return MulNX::Hook::Then::Return;
            }
            this->LogInfo(std::string(s));
            return MulNX::Hook::Then::Continue;
            }) | value_or_throw;
        hook2->Attach();

        });

    this->SubscribeSync("Hook/CSMainLoop", [&](auto&&...) {
        auto clientBase = (uint8_t*)this->CS2->client.GetBaseAddress();

        if (this->pInputSystem->IsKeyPressed('K')) {
            auto a1 = this->FindModule<ClientPanorama>()->FindHudElement("CCSGO_HudChat") - 32;
            using f = __int64 (*)(__int64 a1, const char* a2);
            auto t = clientBase + 0xE7AF90;
            reinterpret_cast<f>(t)(a1, "test");
        }
        static int idx = 0;
        auto a1 = this->FindModule<ParticleManager>();
        if (this->pInputSystem->CheckComboClick('L', 2)) {            
            a1->CreateParticle(&idx,
                "particles/rain_fx/rain_volume_vanity.vpcf", 2, 0, 0, 0, 0);
        }
        if (idx == 0)return;
        auto pawn = this->CS2Entitys->GetLocalPlayerPawnEx();
        if (!pawn)return;
        CS2::CGameSceneNode* node = MulNX::MRead(pawn->pGameSceneNode());
        auto pos = MulNX::MRead(node->vecAbsOrigin());
        a1->UpdateParticle(idx, 0, &pos, 0);
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