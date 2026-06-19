#include "TeamIDController.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXThirdParty/hlae/binutils.h>

using CLayoutFile_LoadFromFile_t = int(__fastcall*)(void*, const char*, unsigned char);

void TeamIDController::HubWindow(MulNX::UINode* node) {
    auto w = MulNX::UI::RAIIWindow("Team ID控制");
    MulNX::UI::Checkbox("Team ID隐藏敌方", this->runFlag1);
}

bool TeamIDController::Init() {
    this->runFlag1.store(true);
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        auto target = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::PosTeamID_CmpForHide);
        auto jmp = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::PosTeamID_xxIt);
        this->hkPosTeamID_CmpForHide = MulNX::Hook::Create(target.Data() + 3, [this](MulNX::Hook* hk, RegContext* ctx) {
            return this->HandleForShowTeamID((CS2::C_CSPlayerPawn*)ctx->rdi);
            }, false, false, (uintptr_t)jmp.Begin()).value();
        this->hkPosTeamID_CmpForHide->Attach();
        this->LogSucc(I18n("hook.attached", "cl_teamid_overhead_maxdist_spec is read here for the comparison to decide Team ID display where rdi is C_CSPlayerPawn*"));
        });

    this->SubscribeSync("Hook/LoadLibraryExW/panorama.dll", [this](MulNX::Message& msg) {
        auto& panorama = this->CS2->panorama;
        auto textRegion = panorama.GetTextRegion();
        auto hMod = panorama.hModule;  // 获取 panorama.dll 句柄

        // Hook CLayoutFile::LoadFromFile
        auto lflAddr = textRegion.FindRegion(
            MulNX::CS2::Signatures::CLayoutFile_LoadFromFile
        );
        if (!lflAddr.IsValid()) return;

        this->hkLoadFromFile = MulNX::Hook::Create(lflAddr.Data(), [this](MulNX::Hook* hk, RegContext* ctx) -> MulNX::Hook::Then {
            // RCX=this, RDX=filePath, R8=unk
            const char* filePath = reinterpret_cast<const char*>(ctx->rdx);
            if (filePath && strstr(filePath, "hudreticle.xml")) {
                this->inHudReticle = true;
                auto result = reinterpret_cast<CLayoutFile_LoadFromFile_t>(hk->pMaybeRawFunc)((void*)ctx->rcx, filePath, ctx->r8);
                *reinterpret_cast<int*>(&ctx->rax) = result;
                this->inHudReticle = false;
                return MulNX::Hook::Then::Return;
            }
            return MulNX::Hook::Then::Continue;
            }
        ).value();
        this->hkLoadFromFile->Attach();
        this->LogSucc(I18n("hook.attached", "CLayoutFile::LoadFromFile"));

        // 获取 CStylePropertyWashColor 的虚表
        void** vtable = (void**)Afx::BinUtils::FindClassVtable(
            (HMODULE)hMod,
            ".?AVCStylePropertyWashColor@panorama@@",
            0, 0
        );
        if (!vtable) return;

        // 从虚表取出 Parse（vtable[6]）和 Clone（vtable[1]）
        auto parseFunc = (void(__fastcall*)(void*, void*, const char*))(vtable[6]);
        auto cloneFunc = (void(__fastcall*)(void*, void*))(vtable[1]);

        // Hook Parse
        this->hkWashColorParse = MulNX::Hook::Create((uint8_t*)parseFunc, [this](MulNX::Hook* hk, RegContext* ctx) -> MulNX::Hook::Then {
            if (!this->inHudReticle) return MulNX::Hook::Then::Continue;

            const char* colorStr = reinterpret_cast<const char*>(ctx->r8);
            uintptr_t objPtr = ctx->rcx;

            if (colorStr && strcmp(colorStr, "#eabe54") == 0) {
                this->tWashColors.insert(objPtr);
            }
            else if (colorStr && strcmp(colorStr, "rgb(150, 200, 250)") == 0) {
                this->ctWashColors.insert(objPtr);
            }
            return MulNX::Hook::Then::Continue;
            }
        ).value();
        this->hkWashColorParse->Attach();
        this->LogSucc(I18n("hook.attached", "CStylePropertyWashColor::Parse"));

        // Hook Clone
        this->hkWashColorClone = MulNX::Hook::Create((uint8_t*)cloneFunc, [this](MulNX::Hook* hk, RegContext* ctx) -> MulNX::Hook::Then {
            if (!this->inHudReticle) return MulNX::Hook::Then::Continue;

            uintptr_t src = ctx->rcx;
            uintptr_t dst = ctx->rdx;

            if (this->tWashColors.count(src)) this->tWashColors.insert(dst);
            if (this->ctWashColors.count(src)) this->ctWashColors.insert(dst);
            return MulNX::Hook::Then::Continue;
            }
        ).value();
        this->hkWashColorClone->Attach();
        this->LogSucc(I18n("hook.attached", "CStylePropertyWashColor::Clone"));
        });

    this->SubscribeSync("Debug/TeamID", [this](MulNX::Message& msg) {
        this->SetCTColor(0, 255, 0, 255);
        this->SetTColor(255, 0, 0, 255);
        });


    return true;
}

// 向单个 WashColor 对象写入颜色 (偏移 +0x10)
void WriteColor(uintptr_t obj, uint32_t rgba) {
    if (obj) *reinterpret_cast<uint32_t*>(obj + 0x10) = rgba;
}
void TeamIDController::SetTColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    // 小端 ARGB 格式：内存中顺序为 B, G, R, A
    uint32_t rgba = (a << 24) | (b << 16) | (g << 8) | r;
    for (auto ptr : this->tWashColors) WriteColor(ptr, rgba);
}

void TeamIDController::SetCTColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    uint32_t rgba = (a << 24) | (b << 16) | (g << 8) | r;
    for (auto ptr : this->ctWashColors) WriteColor(ptr, rgba);
}

void TeamIDController::ResetToDefault() {
    SetTColor(0xEA, 0xBE, 0x54);         // 默认 T： #eabe54
    SetCTColor(150, 200, 250);           // 默认 CT： rgb(150,200,250)
}

MulNX::Hook::Then TeamIDController::HandleForShowTeamID(CS2::C_CSPlayerPawn* pCSPlayerPawn) {
    if (!this->runFlag1.load(std::memory_order_acquire))return MulNX::Hook::Then::Continue;
    try {
        auto pOBPawn = this->CS2->client.TryGetObservingPawn();
        if (!pOBPawn)return MulNX::Hook::Then::Continue;
        auto OBTeam = MulNX::MRead(pOBPawn->iTeamNum());

        auto team = MulNX::MRead(pCSPlayerPawn->iTeamNum());
        if (team == OBTeam) {
            return MulNX::Hook::Then::Continue; // 继续按照旧有距离规则进行判断
        }
        // 跳转到使得迭代器更新的循环尾，继续下一个对象
        // 注意到，这样只是跳过了
        // comiss xmm7,xmm6
        // jb client + rel32
        // 寄存器没有变化，标记位的变化应该也是安全的
        return MulNX::Hook::Then::JmpUserSettedTarget;
    }
    catch (const std::exception& e) {
        this->LogError(std::format("在控制Team ID显示时遇到错误：{}", e.what()));
    }
    return MulNX::Hook::Then::Continue;
}