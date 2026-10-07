#include "CSReShadeController.hpp"

bool CSReShadeController::Init() {
    this->SubscribeSync("Hook/Present/First", [this](auto&&...) {
        auto hBridge = GetModuleHandleW(L"MulNXReShadeBridge.addon");
        if (!hBridge) {
            this->LogInfo("未检测到 MulNXReShadeBridge.addon");
            return;
        }
        auto pCreateInterface = reinterpret_cast<decltype(&CreateInterface)>
            (GetProcAddress(hBridge, "CreateInterface"));
        if (!pCreateInterface) {
            this->LogError("尝试从 MulNXReShadeBridge.addon 中获取 CreateInterface 失败！");
            return;
        }
        pCreateInterface("MulNXReShadeBridge001", &this->pMulNXReShadeBridge);
        if (!this->pMulNXReShadeBridge) {
            this->LogError("CreateInterface 对 MulNXReShadeBridge001 失败！");
            return;
        }
        this->LogSucc("CreateInterface 对 MulNXReShadeBridge001 成功！");
        this->pMulNXReShadeBridge->SetEffectsState(true);
        });

    this->SubscribeSync("GraphicsSync/ID3D11DeviceContext/Ready", [this](MulNX::Message& msg) {
        auto&& [pContext] = msg.Access<ID3D11DeviceContext*>();
        auto t = (uint8_t*)IVClass::Assume(pContext)->GetVFuncPtr(33);
        this->hkOMSetRenderTargets = MulNX::Hook::Create(t, [this](MulNX::Hook* hk, RegContext* ctx) {

            return MulNX::Hook::Then::Continue;
            }).value();
        this->RegisterAttachHook(this->hkOMSetRenderTargets, "OMSetRenderTargets");
        });

    return true;
}