#include "TeamCounterController.hpp"
#include <MulNX/Base/UI/UI.hpp>

using TeamCounter_UpdatePlayerCache = bool (*) (
    __int64 TeamCounterPlayerCache, // 0
    int a2, // 1
    int currentHP, // 2
    int maxHP, // 3
    char a5, // 4
    char a6, // 5
    char a7, // 6
    char a8, // 7
    char a9, // 8
    char a10, // 9
    char a11, // 10
    char a12, // 11
    int a13, // 12
    int n2, // 13
    int a15, // 14
    char bHasBomb, // 15
    char bHasDefuseKit, // 16
    int a18, // 17
    int a19, // 18
    __int16 a20, // 19
    float a21); // 20

void TeamCounterController::Menu() {
    MulNX::UI::Checkbox("隐藏敌方血条", this->hideEnemyHP);
    MulNX::UI::Checkbox("隐藏敌方拆弹器/炸弹", this->hideEnemyDefuseOrKit);
}

bool TeamCounterController::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        auto target = this->CS2->client.GetTextRegion().FindRegion(
            MulNX::CS2::Signatures::Hud::Func_TeamCounterFillPlayerSlotCache).FindFuncStart().Data();
        this->hkTeamCounterFillPlayerSlotCache = MulNX::Hook::Create(target, [this](MulNX::Hook* hk, RegContext* ctx) {
            try {
                this->HandleTeamCounterFillPlayerSlotCacheHook(hk, ctx);
            }
            catch (MulNX::Exception& e) {
                this->LogError(e);
            }
            catch (std::exception& e) {
                this->LogError(e.what());
            }
            catch (...) {
                this->LogError("Unknown Exception");
            }
            return MulNX::Hook::Then::Continue;
            }).value();
        this->RegisterAttachHook(this->hkTeamCounterFillPlayerSlotCache, "Func_TeamCounterFillPlayerSlotCache");
        });

    this->UIRegisterCallback("UI.2DVision", [this](auto&&...) {return this->Menu();});

    return true;
}
void TeamCounterController::HandleTeamCounterFillPlayerSlotCacheHook(MulNX::Hook* hk, RegContext* ctx) {

    auto pOBPawn = this->CS2->client.TryGetObservingPawn();
    if (!pOBPawn)return;
    auto OBingTeam = MulNX::MRead(pOBPawn->iTeamNum());

    auto pCtrl = reinterpret_cast<CS2::CCSPlayerController*>(ctx->r15);
    if (MulNX::MRead(pCtrl->iTeamNum()) == OBingTeam)return;

    if (this->hideEnemyHP) {
        ctx->r8 = 0;
        ctx->r9 = 0;
    }
    if (this->hideEnemyDefuseOrKit) {
        char* p15 = hk->GetStackParam<char>(ctx, 15);
        char* p16 = hk->GetStackParam<char>(ctx, 16);
        *p15 = 0;
        *p16 = 0;
    }

    char* p4 = hk->GetStackParam<char>(ctx, 4);
    char* p5 = hk->GetStackParam<char>(ctx, 5);
    char* p6 = hk->GetStackParam<char>(ctx, 6);
    char* p7 = hk->GetStackParam<char>(ctx, 7);
    char* p8 = hk->GetStackParam<char>(ctx, 8);
    char* p9 = hk->GetStackParam<char>(ctx, 9);
    char* p10 = hk->GetStackParam<char>(ctx, 10);
    char* p11 = hk->GetStackParam<char>(ctx, 11);
    int* p12 = hk->GetStackParam<int>(ctx, 12);
    int* p13 = hk->GetStackParam<int>(ctx, 13);
    int* p14 = hk->GetStackParam<int>(ctx, 14);

    
    int* p17 = hk->GetStackParam<int>(ctx, 17);
    int* p18 = hk->GetStackParam<int>(ctx, 18);
    int16_t* p19 = hk->GetStackParam<int16_t>(ctx, 19);
    float* p20 = hk->GetStackParam<float>(ctx, 20);

    // *p4 = 0;
    // *p5 = 0;
    // *p6 = 0;
    // *p7 = 0;
    // *p8 = 0;
    // *p9 = 0;
    // *p10 = 0;
    // *p11 = 0;

    // *p12 = 0;
    // *p13 = 0;
    // *p14 = 0;
    // *p15 = 0;
    // *p16 = 0;
}