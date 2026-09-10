#include "CS2Test.hpp"
#include <Mirror/ResourceSystem/ResourceSystem.hpp>

ResourceSystem* pResourceSystem = nullptr;

void CS2Test::UI() {
    auto pLocalPawn = this->CS2Entitys->GetLocalPlayerPawnEx();
    if (!pLocalPawn)return;
    auto observerService = MulNX::MRead(pLocalPawn->pObserverServices());
    auto pMode = observerService->iObserverMode();

    uint64_t address = (uint64_t)pMode;  // 示例 64 位地址
    char buf[32];
    snprintf(buf, sizeof(buf), "0x%016llX", address); // 格式化为 16 位十六进制

    ImGui::InputText("Address", buf, sizeof(buf),
        ImGuiInputTextFlags_ReadOnly);
}

bool CS2Test::Init() {
    std::thread([]() {
        MessageBoxW(NULL, L"MulNX 注入成功！", L"MulNX", MB_OK | MB_ICONINFORMATION);
        }).detach();

    pResourceSystem = this->FindModule<ResourceSystem>("ResourceSystem");

    this->AsyncCommand("playdemo 111");
    this->AsyncCommand("tv_listen_voice_indices -1");

    this->SendUIRoot("MyCS2Test", [this](auto&&...) {
        try {
            this->UI();
        }
        catch (MulNX::Exception& e) {

        }
        });

    // this->SubscribeSync("Hook/FireEventClientSide/player_death", [this](MulNX::Message& msg) {
    //     this->runFlag1.store(true);
    //     this->LogWarning("开始记录声音事件");
    //     });

    // this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
    //     static auto hook = MulNX::Hook::Create((uint8_t*)this->CS2->client.GetBaseAddress() + 0xBA34F0, [this](MulNX::Hook* hk, RegContext* ctx) {
    //         auto pppName = hk->GetStackParam<char**>(ctx, 4);
    //         if (this->runFlag1.load()) {
    //             this->LogWarning(std::format("声音：{}",**pppName));
    //         }

    //         return MulNX::Hook::Then::Continue;
    //         }).value();
    //     hook->Attach();
    //     });

    // this->SubscribeSync("Hook/LoadLibraryExW/server.dll", [this](MulNX::Message& msg) {
    //     auto server = MulNX::Memory::DllModule::DllModule(L"server.dll");
    //     auto target = server.GetBaseAddress() + 0xE3D810;
    //     this->hkTest = MulNX::Hook::Create((uint8_t*)target, [this](MulNX::Hook* hk, RegContext* ctx) {
    //         auto pName = **hk->GetStackParam<const char**>(ctx, 4);
    //         auto name = std::string_view(pName);
    //         if (name == "UI.KillCard.1") {
    //             ctx->rax = ctx->rcx;
    //             //return MulNX::Hook::Then::Return;
    //         }
    //         // ctx->rax = ctx->rcx;
    //         // return MulNX::Hook::Then::Return;

    //         return MulNX::Hook::Then::Continue;
    //         }).value();
    //     this->RegisterAttachHook(this->hkTest, "Test");
    //     });

    // this->SubscribeSync("Hook/LoadLibraryExW/engine2.dll", [this](MulNX::Message& msg) {
    //     static auto hook = MulNX::Hook::Create(
    //         (uint8_t*)this->CS2->engine2.GetBaseAddress() + 0x82F86,
    //         [this](MulNX::Hook* hk, RegContext* ctx) {
    //             uintptr_t v51 = ctx->r8;                       // v51 数组首地址
    //             uintptr_t* mapNameSlot = (uintptr_t*)(v51 + 0); // v51[0] 指向 SpawnGroup 名称
    //             const char* currentName = (const char*)*mapNameSlot;
    //             this->LogWarning(std::format("游戏尝试加载：{}", currentName));

    //             if (currentName && strcmp(currentName, "<empty>") != 0) {
    //                 // 情况1：纯地图名（不含路径和 prefabs）
    //                 if (!strchr(currentName, '/') && !strchr(currentName, '\\') && !strstr(currentName, "prefabs")) {
    //                     // 替换主地图名为 de_mirage
    //                     //*mapNameSlot = (uintptr_t)"de_mirage";
    //                     return MulNX::Hook::Then::Continue;
    //                 }

    //                 // 情况2：天空盒（路径中包含 "skybox"）
    //                 if (std::string(currentName).find("skybox") != std::string::npos) {
    //                     // 替换天空盒路径为 de_mirage 的天空盒
    //                     *mapNameSlot = (uintptr_t)"maps/prefabs/de_mirage/3dskybox_mirage_legacy";
    //                     return MulNX::Hook::Then::Continue;
    //                 }
    //             }

    //             // 其它所有情况：跳过（包括 <empty> 和其它 prefabs）
    //             return MulNX::Hook::Then::SkipAllAndContinue;
    //         }, true).value();
    //     hook->Attach();

    //     static auto hook2 = MulNX::Hook::Create(
    //         (uint8_t*)this->CS2->engine2.GetBaseAddress() + 0x842F8,
    //         [this](MulNX::Hook* hk, RegContext* ctx) {
    //             // 此时 r8 指向原始的地图路径字符串（如 "maps/<empty>.vpk" 或 "maps/de_inferno.vpk"）
    //             const char* original = (const char*)ctx->r8;
    //             this->LogWarning(std::format("Hook2 原始地图通知：{}", original ? original : "null"));

    //             // 替换为 de_mirage 的路径
    //             const char* newMapPath = "maps/de_mirage.vpk";
    //             ctx->r8 = (uintptr_t)newMapPath;

    //             return MulNX::Hook::Then::Continue;
    //         }, true).value();
    //     hook2->Attach();
    //     });

    // this->SubscribeSync("Hook/LoadLibraryExW/engine2.dll", [this](MulNX::Message& msg) {
    //     static auto hook3 = MulNX::Hook::Create(
    //         (uint8_t*)this->CS2->engine2.GetBaseAddress() + 0x1AE6E0, // CGameResourceService::BuildResourceManifest
    //         [this](MulNX::Hook* hk, RegContext* ctx) {
    //             // 参数：RCX = this, RDX = a2 (组名，应为地图名如 "de_inferno")
    //             const char* groupName = (const char*)ctx->r9;
    //             this->LogWarning(std::format("Hook3 BuildResourceManifest 组名：{}", groupName ? groupName : "null"));

    //             // 可选：如果你之后想修改，可以在这里替换 ctx->rdx 指向的字符串
    //             // 例如：ctx->rdx = (uintptr_t)"de_mirage";

    //             return MulNX::Hook::Then::Continue;
    //         }).value();
    //     hook3->Attach();
    //     });

    // this->SubscribeSync("Hook/LoadLibraryExW/engine2.dll", [this](MulNX::Message& msg) {
    //     static auto hookManifest = MulNX::Hook::Create(
    //         (uint8_t*)this->CS2->engine2.GetBaseAddress() + 0x3F2689, // call AddString 的地址
    //         [this](MulNX::Hook* hk, RegContext* ctx) {
    //             // 字符串参数在 r8 中
    //             const char* original = (const char*)ctx->r8;
    //             if (original) {
    //                 this->LogWarning(std::format("清单添加字符串：{}", original));

    //                 // 替换地图名（可自定义）
    //                 if (strcmp(original, "de_inferno") == 0) {
    //                     ctx->r8 = (uintptr_t)"de_mirage";
    //                     this->LogWarning("已将地图名替换为 de_mirage");
    //                 }
    //             }
    //             return MulNX::Hook::Then::Continue; // 继续执行原 call
    //         }, true).value();
    //     hookManifest->Attach();
    //     });

    // // 全局缓冲区，用于存放替换后的字符串
    // static char g_ReplacedPath[512];

    // this->SubscribeSync("Hook/LoadLibraryExW/engine2.dll", [this](MulNX::Message& msg) {
    //     static auto hookManifest2 = MulNX::Hook::Create(
    //         (uint8_t*)this->CS2->engine2.GetBaseAddress() + 0x3F2B4D,
    //         [this](MulNX::Hook* hk, RegContext* ctx) {
    //             const char* original = (const char*)ctx->r8;
    //             if (!original) return MulNX::Hook::Then::Continue;

    //             // 只处理包含 de_inferno 的路径
    //             if (strstr(original, "de_inferno")) {
    //                 // 判断是否是需要替换的关键资源
    //                 bool replace = false;
    //                 if (strstr(original, "world") ||
    //                     strstr(original, "skybox") ||
    //                     strstr(original, "postprocessing") ||
    //                     strstr(original, "prefabs") ||
    //                     strstr(original, "pulse")) {
    //                     replace = true;
    //                 }

    //                 // if (replace) {
    //                 //     // 将 de_inferno 替换为 de_mirage
    //                 //     std::string newPath = original;
    //                 //     size_t pos = newPath.find("de_inferno");
    //                 //     while (pos != std::string::npos) {
    //                 //         newPath.replace(pos, strlen("de_inferno"), "de_mirage");
    //                 //         pos = newPath.find("de_inferno", pos + strlen("de_mirage"));
    //                 //     }
    //                 //     // 复制到全局缓冲区
    //                 //     strncpy_s(g_ReplacedPath, newPath.c_str(), sizeof(g_ReplacedPath) - 1);
    //                 //     ctx->r8 = (uintptr_t)g_ReplacedPath;
    //                 //     this->LogWarning(std::format("替换资源路径: {} -> {}", original, g_ReplacedPath));
    //                 // }
    //             }
    //             return MulNX::Hook::Then::Continue;
    //         }, true).value();
    //     hookManifest2->Attach();
    //     });

    // this->SubscribeSync("Hook/LoadLibraryExW/engine2.dll", [this](MulNX::Message& msg) {
    //     static auto hookManifest3 = MulNX::Hook::Create(
    //         (uint8_t*)this->CS2->engine2.GetBaseAddress() + 0x3F26F9, // 第三个 AddString 调用
    //         [this](MulNX::Hook* hk, RegContext* ctx) {
    //             const char* str = (const char*)ctx->r8; // 字符串参数
    //             if (str) {
    //                 this->LogWarning(std::format("清单添加字符串3：{}", str));
    //             }
    //             return MulNX::Hook::Then::Continue;
    //         }, true).value();
    //     hookManifest3->Attach();
    //     });

    // this->SubscribeSync("Hook/LoadLibraryExW/engine2.dll", [this](MulNX::Message& msg) {
    //     // Hook 1: CUtlString::Format("maps/%s.vpk", mapName) 调用处
    //     static auto hookFormatMapPath = MulNX::Hook::Create(
    //         (uint8_t*)this->CS2->engine2.GetBaseAddress() + 0x85162,
    //         [this](MulNX::Hook* hk, RegContext* ctx) {
    //             // r8 是原始地图名字符串（纯地图名，如 "de_inferno"）
    //             // 替换为 de_mirage
    //             ctx->r8 = (uintptr_t)"de_mirage";
    //             this->LogWarning("Hook Format: 已将地图名替换为 de_mirage");
    //             return MulNX::Hook::Then::Continue;
    //         }, true).value();
    //     hookFormatMapPath->Attach();

    //     // Hook 2: CUtlString::Set(mapName) 调用处
    //     static auto hookSetMapName = MulNX::Hook::Create(
    //         (uint8_t*)this->CS2->engine2.GetBaseAddress() + 0x85181,
    //         [this](MulNX::Hook* hk, RegContext* ctx) {
    //             // rdx 是原始地图名字符串（纯地图名，如 "de_inferno"）
    //             // 替换为 de_mirage
    //             ctx->rdx = (uintptr_t)"de_mirage";
    //             this->LogWarning("Hook Set: 已将地图名替换为 de_mirage");
    //             return MulNX::Hook::Then::Continue;
    //         }, true).value();
    //     hookSetMapName->Attach();
    //     });

    return true;
}