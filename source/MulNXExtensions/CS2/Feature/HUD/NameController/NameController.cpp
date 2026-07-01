#include "NameController.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <Buildup/PlayerHub/PlayerHub.hpp>

using GetDecoratedPlayerName_t = const char* (*)(CS2::CCSPlayerController* This_CCSPlayerController,
    char* pBuffer, unsigned int bufferSize, unsigned int maybeShortenLength);

using GetPlayerName_t = const char* (*)(CS2::CCSPlayerController*);

void NameController::HubPlayer(MulNX::UINode* node) {
    std::shared_lock lock(this->smutex);
    auto uid = this->Hub->currentSteamId.load(std::memory_order_acquire);
    auto it = this->nameReplaceInfo.find(uid);
    if (it != this->nameReplaceInfo.end()) {
        ImGui::TextUnformatted(std::format("替换名称: {}", this->nameReplace[it->second]).c_str());
    }
    else {
        ImGui::TextUnformatted("未设置替换名称");
    }
    ImGui::InputText("新名称 (最多127字符)", &this->newNameBuffer);
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

        auto FnGetDecoratedPlayerName = this->CS2->client.GetTextRegion().FindRegion(MulNX::CS2::Signatures::Utils::GetDecoratedPlayerName);
        this->hkGetDecoratedPlayerName = MulNX::Hook::Create(FnGetDecoratedPlayerName.Data(), [this](MulNX::Hook* hk, RegContext* ctx) {
            // 这里注意，这里的名称获取，是需要进一步调用GetPlayerName的
            // 我们借助这一个比较稳定的特征，创建延迟Hook
            // 所以，这里同时不需要加锁，因为它已经满足上下文无关于我们的数据结构的访问了
            // 这里也一定不能加锁，不然可能会被写锁请求打断，导致死锁！
            auto playerController = (CS2::CCSPlayerController*)ctx->rcx;
            this->HandleVHook(playerController);
            return MulNX::Hook::Then::Continue; // 继续执行原始函数，获取装饰名并写入 pBuffer
            }).value();
        this->hkGetDecoratedPlayerName->Attach();
        this->LogSucc(I18n("hook.attached", "GetDecoratedPlayerName"));

        this->SendTask("Update", "CSControl", [this]() {
            this->Update();
            return true;
            });


        });

    return true;
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

void NameController::HandleVHook(CS2::CCSPlayerController* pPlayerController) {
    if (this->bGetPlayerNameHooked)return;
    this->hkGetPlayerName = MulNX::Hook::Create(reinterpret_cast<uint8_t*>(pPlayerController->GetVFuncPtr(226)), [this](MulNX::Hook* hk, RegContext* ctx) {
        // 而在这里，我们则需要加锁，因为我们要访问替换表了
        std::shared_lock lock(this->smutex);

        auto playerController = (CS2::CCSPlayerController*)ctx->rcx;

        // 1. 调用原始函数获取原始名字
        const char* originalName = reinterpret_cast<GetPlayerName_t>(hk->pMaybeRawFunc)(playerController);

        // 2. 获取 SteamID
        uint64_t steamId = *playerController->m_steamID();
        auto it = this->nameReplaceInfo.find(steamId);

        // 3. 根据映射表决定返回值
        if (it != this->nameReplaceInfo.end()) {
            ctx->rax = (uintptr_t)this->nameReplace[it->second];
        }
        else {
            ctx->rax = (uintptr_t)originalName;
        }

        return MulNX::Hook::Then::Return; // 已调用原始函数，不再重复执行
        }).value();
    this->hkGetPlayerName->Attach();
    this->bGetPlayerNameHooked = true;
    this->LogSucc(I18n("hook.attached", "GetPlayerName"));
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