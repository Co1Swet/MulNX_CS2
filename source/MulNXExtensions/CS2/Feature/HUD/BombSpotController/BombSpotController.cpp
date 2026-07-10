#include "BombSpotController.hpp"
#include <MulNX/Base/UI/UI.hpp>

void BombSpotController::Menu(MulNX::UINode* node) {
    MulNX::UI::Checkbox("当观战CT时强制渲染C4为红色", this->runFlag1);
}

bool BombSpotController::Init() {
    this->runFlag1 = true;
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        // 修改雷包颜色
        auto Pos_Spot_WriteBombState = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Spot::Pos_WriteBombState);
        this->hkPos_Spot_WriteBombState = MulNX::Hook::Create(Pos_Spot_WriteBombState.Data(), [this](MulNX::Hook* hk, RegContext* ctx) {
            if (!this->runFlag1.load(std::memory_order_acquire))return MulNX::Hook::Then::Continue;
            auto pOBing = this->CS2->client.TryGetObservingPawn();
            if (!pOBing)return MulNX::Hook::Then::Continue;
            try {
                if (MulNX::MRead(pOBing->iTeamNum()) == CS2::ui8TeamNum::CT) {
                    *(int*)ctx->rdx = IM_COL32(255, 0, 0, 255);// -16776961 red
                }
            }
            catch (...) {

            }
            return MulNX::Hook::Then::Continue;
            }).value();
        this->hkPos_Spot_WriteBombState->Attach();
        this->LogSucc(I18n("hook.attached", "Pos_Spot_WriteBombState where rdx is BombColor*"));

        // auto Pos_CallGetPawnMaybeSetAllHUD = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Spot::Pos_CallGetPawnMaybeSetAllHUD).Data();
        // this->hkPos_CallGetPawnMaybeSetAllHUD = this->CreateHook("Pos_CallGetPawnMaybeSetAllHUD", Pos_CallGetPawnMaybeSetAllHUD + 14, [this](MulNX::Hook* hk, RegContext* ctx) {
        //     if (!this->runFlag2.load())return MulNX::Hook::Then::Continue;
        //     auto pOBing = this->CS2->client.TryGetObservingPawn();
        //     auto pRet = (CS2::C_BaseEntity*)ctx->rax;
        //     //auto name = pRet->GetName();
        //     if (pOBing)ctx->rax = (uint64_t)pOBing;
        //     return MulNX::Hook::Then::Continue;
        //     }).value();
        // this->hkPos_CallGetPawnMaybeSetAllHUD.Attach();

        // auto testpos = this->CS2->client.GetBaseAddress() + 0xBA3546;
        // static auto testhk = MulNX::Hook::Create((uint8_t*)testpos, [](MulNX::Hook*, RegContext* ctx) {
        //     // r13 指向声音事件结构体，其第一个 QWORD 就是字符串指针
        //     char* pStr = *(char**)(ctx->r13);
        //     if (pStr) {
        //         std::string teststr(pStr);
        //         MessageBoxA(NULL, pStr, pStr, MB_OK);
        //         if (teststr.find("Step") != std::string::npos) { // 注意大小写
                    
        //         }
        //     }
        //     return MulNX::Hook::Then::Continue;
        //     }, true).value();
        // testhk->Attach();
        
        });
    this->runFlag2 = true;



    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {return this->Menu(node);});

    return true;
}