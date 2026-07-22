#include "CS2Test.hpp"

bool CS2Test::Init() {
    std::thread([]() {
        MessageBoxW(NULL, L"MulNX 注入成功！", L"MulNX", MB_OK | MB_ICONINFORMATION);
        }).detach();
    this->AsyncCommand("playdemo 111");
    this->AsyncCommand("tv_listen_voice_indices -1");

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

    return true;
}