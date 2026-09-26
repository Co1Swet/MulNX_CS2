#include "NameController.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <Support/PlayerHub/PlayerHub.hpp>
#include <MulNXThirdParty/hlae/binutils.h>

using GetDecoratedPlayerName_t = void(*)(void* This, CS2::CBufferString* pBufferString, unsigned int flags, bool bUnk3);
using GetPlayerName_t = const char* (*)(CS2::CCSPlayerController*);

void NameController::UIPlayer(MulNX::Message* msg) {
    std::shared_lock lock(this->smutex);
    auto [uid] = msg->Access<Steam64UID>();
    auto it = this->nameReplaceInfo.find(uid);
    if (it != this->nameReplaceInfo.end()) {
        ImGui::TextUnformatted(std::format("替换名称: {}", this->nameReplace[it->second]).c_str());
    }
    else {
        ImGui::TextUnformatted("未设置替换名称");
    }
    ImGui::InputText("新名称 (最多127字符)", &this->newNameBuffer);
    MulNX::UI::Checkbox("屏蔽名称前缀（所有人）", this->noClantag);
    ImGui::SameLine();
    if (ImGui::Button("设定（空则清除）")) {
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Name/Player/Set"_hash);
        auto&& [uidRef] = msg.Access<Steam64UID>();
        uidRef = uid;
        rp->str1 = this->newNameBuffer;
        this->PublishAsync(std::move(msg));
        this->newNameBuffer.clear();
    }
}

bool NameController::Init() {
    this->SubscribeAsync("Name/Player/Set");

    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {

        auto pFnGetDecoratedPlayerName = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Utils::GetDecoratedPlayerName).Data();
        this->hkGetDecoratedPlayerName = MulNX::Hook::Create(pFnGetDecoratedPlayerName, [this](MulNX::Hook* hk, RegContext* ctx) {
            try {
                return this->HandleGetDecoratedPlayerName(hk, ctx);
            }
            catch (MulNX::Exception& e) {
                this->LogError(e);
            }
            return MulNX::Hook::Then::SkipAllAndContinue;
            }, true, true).value();
        this->RegisterAttachHook(this->hkGetDecoratedPlayerName,
            "PosInFunc_GetDecoratedPlayerName");

        // fn has 3rd reference to string "WWWWWWWWWWWWWWWW"
        uint8_t** vtable = (uint8_t**)Afx::BinUtils::FindClassVtable(this->CS2->client.hModule, ".?AVCCSPlayerController@@", 0, 0);
        if (!vtable)MulNX::ErrorTerminate("找不到pCCSPlayerController::vtable");
        
        auto pCCSPlayerController_GetPlayerName = vtable[230];
        this->hkGetPlayerName = MulNX::Hook::Create(pCCSPlayerController_GetPlayerName, [this](MulNX::Hook* hk, RegContext* ctx) {
            auto playerController = (CS2::CCSPlayerController*)ctx->rcx;
            // 调用原始函数获取原始名字
            ctx->rax = (uint64_t)reinterpret_cast<GetPlayerName_t>(hk->pMaybeRawFunc)(playerController);
            // 获取 SteamID
            uint64_t steamId = *playerController->m_steamID();
            // 而在这里，我们则需要加锁，因为我们要访问替换表了
            std::shared_lock lock(this->smutex);
            auto it = this->nameReplaceInfo.find(steamId);

            // 根据映射表决定返回值
            if (it != this->nameReplaceInfo.end()) {
                ctx->rax = (uintptr_t)this->nameReplace[it->second];
            }

            return MulNX::Hook::Then::Return; // 已调用原始函数，不再重复执行
            }).value();
        this->RegisterAttachHook(this->hkGetPlayerName, "GetPlayerName");

        this->SendTask("Update", "CSControl", [this]() {
            this->Update();
            return true;
            });


        });

    this->UIRegisterCallback("UI.Player.Info", [this](auto, auto msg) {return this->UIPlayer(msg);});

    return true;
}

MulNX::Hook::Then NameController::HandleGetDecoratedPlayerName(MulNX::Hook* hk, RegContext* ctx) {
    auto ppName = (const char**)&ctx->rax;

    auto pProvider = (ctx->r12);
    // (int* (__fastcall*)(void*, int*))(vtable[7]);
    auto GetUserId = IVClass::Assume(pProvider)->GetVFunc<int* (int*)>(7);
    int userId = -1;
    GetUserId(&userId);
    if (userId == -1)return MulNX::Hook::Then::SkipAllAndContinue;

    auto pCtrler = this->CS2Entitys->GetBaseEntity(userId + 1)->As<CS2::CBasePlayerController>();
    auto steamId = MulNX::MRead(pCtrler->m_steamID());

    const char* currentComponentName = *(const char**)ctx->rdi;

    if (*currentComponentName == 'o') {
        // original_controller
        std::shared_lock lock(this->smutex);
        auto it = this->nameReplaceInfo.find(steamId);
        // 根据映射表决定返回值
        if (it != this->nameReplaceInfo.end()) {
            *ppName = this->nameReplace[it->second];
        }
    }
    else if (*currentComponentName == 'c') {
        // clantag
        if (this->noClantag.load()) {
            *ppName = nullptr;
        }
    }
    else if (*currentComponentName == 'p') {
        // puppeteer
    }
    

    return MulNX::Hook::Then::SkipAllAndContinue; // 继续执行原始函数，获取装饰名并写入 pBuffer
}

void NameController::ProcessMsg(MulNX::Message& Msg) {
    switch (Msg.type) {
    case "Name/Player/Set"_hash: {
        auto&& [uid] = Msg.Access<Steam64UID>();
        auto newName = Msg.asp.get<MulNX::NetExt>()->str1;
        std::unique_lock lock(this->smutex);
        this->SetReplace(uid, newName);
        break;
    }
    default:
        break;
    }
}

bool NameController::SetReplace(Steam64UID uid, const std::string& newName) {
    if (newName.empty()) {
        auto it = this->nameReplaceInfo.find(uid);
        if (it == this->nameReplaceInfo.end()) {
            this->LogError("无法删除不存在的替换规则！");
            return false;
        }
        int idx = it->second;
        this->nameReplaceInfo.erase(it);
        memset(this->nameReplace[idx], 0, 128);   // 清空槽位
        this->LogInfo(std::format("已删除 SteamID {} 的名称替换规则", uid));
        return true;
    }

    if (newName.size() >= 128) {
        this->LogError("名称长度不能超过127个字符！");
        return false;
    }

    auto it = this->nameReplaceInfo.find(uid);
    int idx = -1;

    if (it != this->nameReplaceInfo.end()) {
        // 更新现有条目
        idx = it->second;
    }
    else {
        // 寻找空闲索引
        for (int i = 0; i < 64; ++i) {
            bool used = false;
            for (auto& pair : this->nameReplaceInfo) {
                if (pair.second == i) { used = true; break; }
            }
            if (!used) { idx = i; break; }
        }
        if (idx == -1) {
            this->LogError("名称替换槽位已满 (最多64条)！");
            return false;
        }
        this->nameReplaceInfo[uid] = idx;
    }

    // 安全复制字符串
    strncpy_s(this->nameReplace[idx], newName.c_str(), 127);
    this->nameReplace[idx][127] = '\0';
    this->LogInfo(std::format("已为 SteamID {} 设置替换名称: {}", uid, newName));

    return true;
}