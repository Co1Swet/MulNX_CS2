#include "TeamIDController.hpp"
#include <MulNXExtensions/CS2/CSController/CSController.hpp>
#include <MulNXThirdParty/hlae/binutils.h>  // 假定你已集成了 HLAE 的 binutils

bool TeamIDController::Init() {
    auto& panorama = this->CS2()->panorama;
    auto textRegion = panorama.GetTextRegion();
    auto hMod = panorama.hModule;  // 获取 panorama.dll 句柄（需确保你的 CS2 类型有此方法）

    // 1. Hook CLayoutFile::LoadFromFile
    auto lflAddr = textRegion.FindRegion(
        MulNX::CS2::Signatures::CLayoutFile_LoadFromFile
    );
    if (!lflAddr.IsValid()) return false;

    hkLoadFromFile_ = MulNX::Hook::Create(lflAddr.Data(), 0, false,
        [this](RegContext* ctx, MulNX::Hook* hk) -> MulNX::Hook::Then {
            // __fastcall: RCX=this, RDX=filePath, R8=unk
            const char* filePath = reinterpret_cast<const char*>(ctx->rdx);
            if (filePath && strstr(filePath, "hudreticle.xml")) {
                inHudReticle_ = true;
                //hk->CallOriginal();      // 内部会触发 Parse / Clone
                inHudReticle_ = false;
                //return MulNX::Hook::Then::Skip; // 已手动调用原始，跳过默认执行
            }
            return MulNX::Hook::Then::Continue;
        }
    ).value();
    hkLoadFromFile_->Attach();

    // 2. 获取 CStylePropertyWashColor 的虚表
    void** vtable = (void**)Afx::BinUtils::FindClassVtable(
        (HMODULE)hMod,
        ".?AVCStylePropertyWashColor@panorama@@",
        0, 0
    );
    if (!vtable) return false;

    // 3. 从虚表取出 Parse（vtable[6]）和 Clone（vtable[1]）
    auto parseFunc = (void(__fastcall*)(void*, void*, const char*))(vtable[6]);
    auto cloneFunc = (void(__fastcall*)(void*, void*))(vtable[1]);

    // 4. Hook Parse
    hkWashColorParse_ = MulNX::Hook::Create((uint8_t*)parseFunc, 0, false,
        [this](RegContext* ctx, MulNX::Hook* hk) -> MulNX::Hook::Then {
            if (!inHudReticle_) return MulNX::Hook::Then::Continue;

            const char* colorStr = reinterpret_cast<const char*>(ctx->r8);
            uintptr_t objPtr = ctx->rcx;

            if (colorStr && strcmp(colorStr, "#eabe54") == 0) {
                tWashColors_.insert(objPtr);
            }
            else if (colorStr && strcmp(colorStr, "rgb(150, 200, 250)") == 0) {
                ctWashColors_.insert(objPtr);
            }
            return MulNX::Hook::Then::Continue;
        }
    ).value();
    hkWashColorParse_->Attach();

    // 5. Hook Clone
    hkWashColorClone_ = MulNX::Hook::Create((uint8_t*)cloneFunc, 0, false,
        [this](RegContext* ctx, MulNX::Hook* hk) -> MulNX::Hook::Then {
            if (!inHudReticle_) return MulNX::Hook::Then::Continue;

            uintptr_t src = ctx->rcx;
            uintptr_t dst = ctx->rdx;   // __fastcall 第二个参数通过 RDX

            if (tWashColors_.count(src)) tWashColors_.insert(dst);
            if (ctWashColors_.count(src)) ctWashColors_.insert(dst);
            return MulNX::Hook::Then::Continue;
        }
    ).value();
    hkWashColorClone_->Attach();

    // 6. 若为后期注入，强制触发一次 HUD 重新加载以捕获对象
    // 可根据需要启用，例如：
    // this->CS2()->ExecuteClientCmd("cl_reload_hud");

    return true;
}

void TeamIDController::SetTColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    // 小端 ARGB 格式：内存中顺序为 B, G, R, A
    uint32_t rgba = (a << 24) | (b << 16) | (g << 8) | r;
    for (auto ptr : tWashColors_) WriteColor(ptr, rgba);
}

void TeamIDController::SetCTColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    uint32_t rgba = (a << 24) | (b << 16) | (g << 8) | r;
    for (auto ptr : ctWashColors_) WriteColor(ptr, rgba);
}

void TeamIDController::ResetToDefault() {
    SetTColor(0xEA, 0xBE, 0x54);         // 默认 T： #eabe54
    SetCTColor(150, 200, 250);           // 默认 CT： rgb(150,200,250)
}