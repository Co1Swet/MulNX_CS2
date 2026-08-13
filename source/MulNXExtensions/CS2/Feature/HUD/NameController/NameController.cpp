#include "NameController.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <Buildup/PlayerHub/PlayerHub.hpp>
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

// .text:00000001808C24A0 4C 89 A5 98 00 00 00                                            mov[rbp + 210h + var_178], r12
// .text:00000001808C24A7 4C 89 74 24 58                                                  mov[rsp + 310h + var_2B8], r14
// .text:00000001808C24AC F3 0F 7F 44 24 60                                               movdqu[rsp + 310h + var_2B0], xmm0
// .text:00000001808C24B2
// .text:00000001808C24B2                                                 loc_1808C24B2 : ; CODE XREF : sub_1808C2260 + 34F↓j
// .text : 00000001808C24B2 44 85 7F F8                                                     test[rdi - 8], r15d
// .text:00000001808C24B6 0F 84 C9 00 00 00                                               jz      loc_1808C2585
// .text:00000001808C24BC 48 8B 4F 40                                                     mov     rcx, [rdi + 40h]
// .text:00000001808C24C0 48 85 C9                                                        test    rcx, rcx
// .text:00000001808C24C3 0F 84 35 04 00 00                                               jz      loc_1808C28FE
// .text:00000001808C24C9 48 8B 01                                                        mov     rax, [rcx] <--- 我们在这里进行劫持
// .text:00000001808C24CC FF 50 10                                                        call    qword ptr[rax + 10h]
// .text:00000001808C24CF 4C 8B F0                                                        mov     r14, rax <--- 自由篡改字符串！
// .text:00000001808C24D2 48 85 C0                                                        test    rax, rax
// .text:00000001808C24D5 0F 84 A7 00 00 00                                               jz      loc_1808C2582
// .text:00000001808C24DB 80 38 00                                                        cmp     byte ptr[rax], 0
// .text:00000001808C24DE 0F 84 9E 00 00 00                                               jz      loc_1808C2582
// .text:00000001808C24E4 48 8B 1F                                                        mov     rbx, [rdi] <--- 这个地方不稳定，我们不要通过rbx拿取现在正在读取什么（拿了也是垃圾值）
// .text:00000001808C24E7 48 8D 8D 38 02 00 00                                            lea     rcx, [rbp + 210h + arg_18]
// .text:00000001808C24EE 48 8B D3                                                        mov     rdx, rbx
// .text:00000001808C24F1 FF 15 91 61 09 01                                               call    cs : ? Make@CUtlStringToken@@SA ? AV1@PEBD@Z; CUtlStringToken::Make(char const*)
// .text:00000001808C24F7 48 85 DB                                                        test    rbx, rbx
// .text:00000001808C24FA C6 44 24 34 00                                                  mov[rsp + 310h + var_2DC], 0
// .text:00000001808C24FF C7 44 24 30 01 00 00 00                                         mov[rsp + 310h + var_2E0], 1
// .text:00000001808C2507 8B 08                                                           mov     ecx, [rax]
// .text:00000001808C2509 48 8D 05 78 7B 0A 01                                            lea     rax, byte_18196A088
// .text:00000001808C2510 48 0F 45 C3                                                     cmovnz  rax, rbx
// .text:00000001808C2514 89 4C 24 40                                                     mov[rsp + 310h + var_2D0], ecx
// .text:00000001808C2518 49 8B CE                                                        mov     rcx, r14
// .text:00000001808C251B 48 89 44 24 48                                                  mov[rsp + 310h + var_2C8], rax
// .text:00000001808C2520 FF 15 FA 62 09 01                                               call    cs : MemAlloc_StrDupFunc
// .text : 00000001808C2526 45 33 C9 xor r9d, r9d
// .text:00000001808C2529 4C 8D 44 24 30                                                  lea     r8, [rsp + 310h + var_2E0]
// .text:00000001808C252E 48 8D 54 24 40                                                  lea     rdx, [rsp + 310h + var_2D0]

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
            "PosInFunc_GetDecoratedPlayerName where r12 is *provider and rdi is **currentComponentName(char**) and rax is **tempRetName(char**)");

        // fn has 3rd reference to string "WWWWWWWWWWWWWWWW"
        uint8_t** vtable = (uint8_t**)Afx::BinUtils::FindClassVtable(this->CS2->client.hModule, ".?AVCCSPlayerController@@", 0, 0);
        if (!vtable)MulNX::ErrorTerminate("找不到pCCSPlayerController::vtable");
        
        auto pCCSPlayerController_GetPlayerName = vtable[226];
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

    auto pCtrler = this->CS2->client.GetBaseEntity(userId + 1)->As<CS2::CBasePlayerController>();
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