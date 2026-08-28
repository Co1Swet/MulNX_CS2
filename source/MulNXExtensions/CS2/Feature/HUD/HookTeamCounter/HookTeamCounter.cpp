#include "HookTeamCounter.hpp"
#include <MulNX/Base/UI/UI.hpp>

using TeamCounter_UpdatePlayerCache = bool (*) (
    __int64 TeamCounterPlayerCache, // 0
    int a2, // 1
    int iHealth, // 2
    int iArmor, // 3
    char bLeft, // 4 这个变量具体作用不清楚，当它置为1，teamcounter只有左半边的绘制，当它置为0，只有右半边，而且我观察到它闪烁，疑似依赖这个变量定位槽位
    char m_bIsLocalPlayerController, // 5
    char bDisableColor, // 6 很奇怪的效果
    char bOverlayARedCycle, // 7 置1可以在头像上附加一个有四个向内延伸线的红圈，像瞄准镜
    char bOverlayARedPerson, // 8 置1可以在头像上画一个红色人的“证件照”
    char bIsSpeaking, // 9 顾名思义是那个说话时在头像上的小喇叭
    char bIsControllingThisBot, // 10 置1叠加正在控制这个bot的标识
    char bIsBeingSelected, // 11 正在被观战，会亮起玩家的颜色光
    int unk12, // 12
    int n2, // 13
    int iColorIndex, // 14   0蓝色 1绿色 2黄色 3橙色 4紫色 循环往复
    char bHasBomb, // 15
    char bHasDefuseKit, // 16
    int iMoney, // 17
    int unk18, // 18
    __int16 iWeaponIconIndex, // 19 应该是一个指向显示什么武器图标的短索引
    float unk20); // 20 一个随不同Demo时间全员一致的浮点数，在0到1000变化

void HookTeamCounter::Menu() {
    ImGui::SeparatorText("TeamCounter(HUD上方玩家信息)");
    MulNX::UI::Checkbox("隐藏敌方血条", this->hideEnemyHP);
    MulNX::UI::Checkbox("隐藏敌方拆弹器/炸弹", this->hideEnemyDefuseOrKit);
    MulNX::UI::Checkbox("强制显示名字", this->forceShowName);
    MulNX::UI::Checkbox("强制隐藏装备信息", this->forceHideEquipmentInfo);
    ImGui::Separator();
}

bool HookTeamCounter::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        auto target = this->CS2->client.GetTextRegion().FindRegion(
            MulNX::CS2::Signatures::Hud::TeamCounter::Func_FillPlayerSlotCache).FindFuncStart().Data();
        this->hkTeamCounterFillPlayerSlotCache = MulNX::Hook::Create(target, [this](MulNX::Hook* hk, RegContext* ctx) {
            try {
                this->HandleTeamCounterFillPlayerSlotCacheHook(hk, ctx);
            }
            catch (MulNX::Exception& e) {
                this->LogError(e);
            }
            return MulNX::Hook::Then::Continue;
            }).value();
        this->RegisterAttachHook(this->hkTeamCounterFillPlayerSlotCache, "Func_TeamCounterFillPlayerSlotCache where r15 is *CCSPlayerController");

        auto tPos_UpdatePanoramaFullInfoVisible = this->CS2->client.GetTextRegion().FindRegion(
            MulNX::CS2::Signatures::Hud::TeamCounter::Pos_UpdatePanoramaFullInfoVisible).Data();
        this->hkPos_UpdatePanoramaFullInfoVisible = MulNX::Hook::Create(tPos_UpdatePanoramaFullInfoVisible, [this](MulNX::Hook* hk, RegContext* ctx) {
            try {
                if (this->forceHideEquipmentInfo) {
                    ctx->r8 = false;
                    return MulNX::Hook::Then::Continue;
                }
                auto pGameRules = this->CS2->client.dwGameRules();
                if (!pGameRules)return MulNX::Hook::Then::Continue;
                auto bIsFreeze = MulNX::MRead(&pGameRules->m_bFreezePeriod);
                auto flRestartRoundTime = MulNX::MRead(&pGameRules->m_flRestartRoundTime);
                if (!bIsFreeze && !flRestartRoundTime) {
                    ctx->r8 = false;
                    return MulNX::Hook::Then::Continue;
                }
                auto pOBingPawn = this->CS2Entitys->TryGetObservingPawn();
                if (!pOBingPawn)return MulNX::Hook::Then::Continue;
                auto pCtrl = (CS2::CCSPlayerController*)ctx->rdi;
                auto OBTeam = MulNX::MRead(pOBingPawn->iTeamNum());
                auto Team = MulNX::MRead(pCtrl->iTeamNum());
                if (OBTeam == Team)return MulNX::Hook::Then::Continue;
                ctx->r8 = false;
            }
            catch (const MulNX::Exception& e) {
                this->LogError(e);
            }
            return MulNX::Hook::Then::Continue;
            }, true).value();
        this->RegisterAttachHook(this->hkPos_UpdatePanoramaFullInfoVisible, "Pos_UpdatePanoramaFullInfoVisible where rdi is CCSPlayerController*");

        auto tPos_UpdatePanoramaNameVisible = this->CS2->client.GetTextRegion().FindRegion(
            MulNX::CS2::Signatures::Hud::TeamCounter::Pos_UpdatePanoramaNameVisible).Data();
        this->hkPos_UpdatePanoramaNameVisible = MulNX::Hook::Create(tPos_UpdatePanoramaNameVisible, [this](MulNX::Hook* hk, RegContext* ctx) {
            try {
                if (this->forceShowName) {
                    ctx->r8 = true;
                    return MulNX::Hook::Then::Continue;
                }
                auto pGameRules = this->CS2->client.dwGameRules();
                if (!pGameRules)return MulNX::Hook::Then::Continue;
                auto bIsFreeze = MulNX::MRead(&pGameRules->m_bFreezePeriod);
                if (bIsFreeze)return MulNX::Hook::Then::Continue;
                ctx->r8 = false;
            }
            catch (const MulNX::Exception& e) {
                this->LogError(e);
            }
            return MulNX::Hook::Then::Continue;
            }, true).value();
        this->RegisterAttachHook(this->hkPos_UpdatePanoramaNameVisible, "Pos_UpdatePanoramaNameVisible");

        auto tPos_UpdatePanoramaSpecTargetVisible = this->CS2->client.GetTextRegion().FindRegion(
            MulNX::CS2::Signatures::Hud::TeamCounter::Pos_UpdatePanoramaSpecTargetVisible).Data() + 7;
        this->hkPos_UpdatePanoramaSpecTargetVisible = MulNX::Hook::Create(tPos_UpdatePanoramaSpecTargetVisible, [this](MulNX::Hook* hk, RegContext* ctx) {
            ctx->r8 = false;
            return MulNX::Hook::Then::Continue;
            }, true).value();
        this->RegisterAttachHook(this->hkPos_UpdatePanoramaSpecTargetVisible, "Pos_UpdatePanoramaSpecTargetVisible");
        });

    this->UIRegisterCallback("UI.2DVision", [this](auto&&...) {return this->Menu();});

    return true;
}
void HookTeamCounter::HandleTeamCounterFillPlayerSlotCacheHook(MulNX::Hook* hk, RegContext* ctx) {
    auto pOBPawn = this->CS2Entitys->TryGetObservingPawn();
    if (!pOBPawn)return;
    auto OBingTeam = MulNX::MRead(pOBPawn->iTeamNum());

    auto pCtrl = reinterpret_cast<CS2::CCSPlayerController*>(ctx->r15);
    if (MulNX::MRead(pCtrl->iTeamNum()) == OBingTeam)return;

    if (this->hideEnemyHP) {
        ctx->r8 = 0;
    }
    if (this->hideEnemyDefuseOrKit) {
        char* p15 = hk->GetStackParam<char>(ctx, 15);
        char* p16 = hk->GetStackParam<char>(ctx, 16);
        *p15 = 0;
        *p16 = 0;
    }
#ifdef _DEBUG
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

    //*p18 = 0;

    // *p19 = 3;

    // *p4 = 1;
    // *p5 = 1;
    // *p6 = 2;
    // *p7 = 1;
    // *p8 = 1;
    // *p9 = 1;
    // *p10 = 0;
    // *p11 = 1;

    // *p12 = 0;
    // *p13 = 0b1101;
    // *p14 = 0;
    // *p15 = 0;
    // *p16 = 0;
#endif // _DEBUG
}