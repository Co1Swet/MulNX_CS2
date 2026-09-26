#include "CS2Test.hpp"
#include <MulNXUtils/AddressCheck.hpp>
#include <unordered_set>

void CS2Test::UI() {
    auto pLocalPawn = this->CS2Entitys->GetLocalPlayerPawnEx();
    if (!pLocalPawn)return;
    auto observerService = MulNX::MRead(pLocalPawn->pObserverServices());
    auto pMode = observerService->iObserverMode();

    uint64_t address = (uint64_t)pMode;
    char buf[32];
    snprintf(buf, sizeof(buf), "0x%016llX", address);

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

    return true;
}