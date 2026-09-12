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

    

    return true;
}