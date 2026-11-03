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
        uint64_t offset = 0x76C260;
        auto base = (uint8_t*)this->CS2->client.GetBaseAddress();
        auto addr = (uint8_t*)(base + offset);
        static auto hook=MulNX::Hook::Create(addr,[this](MulNX::Hook* hk,RegContext* ctx)->MulNX::Hook::Then {
            auto a3 = (uint32_t*)ctx->r8;
            auto v = *a3;
            if (v == 4) {
                this->LogInfo("4");
            }
            if (v == 5) {
                this->LogInfo("5");
                *a3 = 3;
                ctx->rax = 3;
                return MulNX::Hook::Then::Return;
            }
            

            return MulNX::Hook::Then::Continue;
            }).value();
        hook->Attach();

        static auto hook2 = MulNX::Hook::Create(base + 0xDAD422, [this](MulNX::Hook* hk, RegContext* ctx)->MulNX::Hook::Then {
            if (ctx->rax == 5) {
                this->LogInfo("5");
                ctx->rax = 3;
            }
            return MulNX::Hook::Then::Continue;
            }, true).value();
        hook2->Attach();

        static auto hook3 = MulNX::Hook::Create(base + 0x76C3E0, [this](MulNX::Hook* hk, RegContext* ctx) -> MulNX::Hook::Then {
            auto self = (uint8_t*)ctx->rcx;       // C_CSGameRules*
            auto a3 = (uint8_t*)ctx->r8;          // &new m_nMatchEndCount
            // 方案：新值改成旧值，条件不成立，不派发 cs_match_end_restart
            *a3 = *(uint8_t*)(self + 3840);
            return MulNX::Hook::Then::Continue;
            }).value();
        hook3->Attach();

        static auto hookShow = MulNX::Hook::Create(base + 0xDBA920, [this](MulNX::Hook* hk, RegContext* ctx) -> MulNX::Hook::Then {
            return MulNX::Hook::Then::Return;   // 整个函数不执行
            }).value();
        hookShow->Attach();

        });

    return true;
}