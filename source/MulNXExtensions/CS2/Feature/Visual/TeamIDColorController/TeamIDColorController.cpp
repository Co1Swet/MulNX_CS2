#include "TeamIDColorController.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXThirdParty/hlae/binutils.h>
#include <Buildup/PlayerHub/PlayerHub.hpp>

using CLayoutFile_LoadFromFile_t = int(__fastcall*)(void*, const char*, unsigned char);

void TeamIDColorController::HubTeam(MulNX::Message* umsg) {
    std::shared_lock lock(this->smutex);
    auto&& [team] = umsg->Access<CS2::ui8TeamNum>();

    ImVec4& colorVec4 = (team == CS2::ui8TeamNum::T) ? this->bufferTColor : this->bufferCTColor;
    if (team != CS2::ui8TeamNum::T && team != CS2::ui8TeamNum::CT) {
        ImGui::Text("当前队伍无效，无法修改头顶颜色");
        return;
    }

    if (ImGui::ColorEdit4("头顶颜色修改", (float*)&colorVec4)) {
        // 手动构建大端 RGBA 消息：最高字节 R，次高 G，次低 B，最低 A
        uint8_t r = (uint8_t)(colorVec4.x * 255.0f);
        uint8_t g = (uint8_t)(colorVec4.y * 255.0f);
        uint8_t b = (uint8_t)(colorVec4.z * 255.0f);
        uint8_t a = (uint8_t)(colorVec4.w * 255.0f);
        uint32_t newColorU32 = ((uint32_t)r << 24) | ((uint32_t)g << 16) | ((uint32_t)b << 8) | (uint32_t)a;

        auto msgType = (team == CS2::ui8TeamNum::T) ? "TeamIDColor/T/Set"_hash : "TeamIDColor/CT/Set"_hash;
        MulNX::Message msg(msgType);
        auto&& [newColorRef] = msg.Access<uint32_t>();
        newColorRef = newColorU32;
        this->PublishAsync(std::move(msg));
    }

    ImGui::SameLine();
    if (ImGui::Button("重置头顶颜色")) {
        auto msgType = (team == CS2::ui8TeamNum::T) ? "TeamIDColor/T/Reset"_hash : "TeamIDColor/CT/Reset"_hash;
        MulNX::Message msg(msgType);
        this->PublishAsync(std::move(msg));
    }
}

bool TeamIDColorController::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/panorama.dll", [this](MulNX::Message& msg) {
        auto& panorama = this->CS2->panorama;
        auto textRegion = panorama.GetTextRegion();
        auto hMod = panorama.hModule;  // 获取 panorama.dll 句柄

        // Hook CLayoutFile::LoadFromFile
        auto lflAddr = textRegion.FindRegion(
            MulNX::CS2::Signatures::Hud::CLayoutFile_LoadFromFile
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
            WashColor* objPtr = reinterpret_cast<WashColor*>(ctx->rcx);

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

            WashColor* src = reinterpret_cast<WashColor*>(ctx->rcx);
            WashColor* dst = reinterpret_cast<WashColor*>(ctx->rdx);

            if (this->tWashColors.count(src)) this->tWashColors.insert(dst);
            if (this->ctWashColors.count(src)) this->ctWashColors.insert(dst);
            return MulNX::Hook::Then::Continue;
            }
        ).value();
        this->hkWashColorClone->Attach();
        this->LogSucc(I18n("hook.attached", "CStylePropertyWashColor::Clone"));
        });

    (*this)
        .SubscribeAsync<uint32_t>("TeamIDColor/T/Set")
        .SubscribeAsync<uint32_t>("TeamIDColor/CT/Set")
        .SubscribeAsync<void>("TeamIDColor/T/Reset")
        .SubscribeAsync<void>("TeamIDColor/CT/Reset")
        ;

    this->SendTask("Update", "CSControl", [this]() {
        this->Update();
        return true;
        });

    this->UIRegisterCallback("UI.Team.Info", [this](auto, auto msg) {this->HubTeam(msg);});

    return true;
}

void TeamIDColorController::SetTColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    // 小端 ARGB 格式：内存中顺序为 B, G, R, A
    uint32_t rgba = (a << 24) | (b << 16) | (g << 8) | r;
    for (auto ptr : this->tWashColors) ptr->color = rgba;
}

void TeamIDColorController::SetCTColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    uint32_t rgba = (a << 24) | (b << 16) | (g << 8) | r;
    for (auto ptr : this->ctWashColors) ptr->color = rgba;
}

void TeamIDColorController::ProcessMsg(MulNX::Message& Msg) {
    switch (Msg.type) {
    case "TeamIDColor/T/Set"_hash: {
        auto&& [rgba] = Msg.Access<uint32_t>();
        // 大端 RGBA 消息：R=最高字节, G=次高, B=次低, A=最低
        uint8_t r = (rgba >> 24) & 0xFF;
        uint8_t g = (rgba >> 16) & 0xFF;
        uint8_t b = (rgba >> 8) & 0xFF;
        uint8_t a = rgba & 0xFF;

        std::unique_lock lock(this->smutex);
        this->bufferTColor = ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
        lock.unlock();
        this->SetTColor(r, g, b, a);
        break;
    }
    case "TeamIDColor/CT/Set"_hash: {
        auto&& [rgba] = Msg.Access<uint32_t>();
        uint8_t r = (rgba >> 24) & 0xFF;
        uint8_t g = (rgba >> 16) & 0xFF;
        uint8_t b = (rgba >> 8) & 0xFF;
        uint8_t a = rgba & 0xFF;

        std::unique_lock lock(this->smutex);
        this->bufferCTColor = ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
        lock.unlock();
        this->SetCTColor(r, g, b, a);
        break;
    }
    case "TeamIDColor/T/Reset"_hash: {
        std::unique_lock lock(this->smutex);
        this->bufferTColor = ImVec4(0xEA / 255.0f, 0xBE / 255.0f, 0x54 / 255.0f, 1.0f);
        lock.unlock();
        this->ResetTColor();
        break;
    }
    case "TeamIDColor/CT/Reset"_hash: {
        std::unique_lock lock(this->smutex);
        this->bufferCTColor = ImVec4(0x96 / 255.0f, 0xC8 / 255.0f, 0xFA / 255.0f, 1.0f);
        lock.unlock();
        this->ResetCTColor();
        break;
    }
    }
}