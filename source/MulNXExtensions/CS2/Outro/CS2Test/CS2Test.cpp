#include "CS2Test.hpp"

void CS2Test::UI() {
    auto pLocalPawn = this->CS2->client.GetLocalPlayerPawn();
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

    return true;
}