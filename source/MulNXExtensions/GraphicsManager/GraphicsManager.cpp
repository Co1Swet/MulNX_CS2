#include "GraphicsManager.hpp"

bool MulNX::GraphicsManager::Init() {

    this->SubscribeSync("Hook/Present", [this](auto&&...) {
        if (this->refBackBufferView) return;
        ComPtr<ID3D11Texture2D> buf = nullptr;
        this->pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&buf);
        if (!buf) return;
        this->pd3dDevice->CreateRenderTargetView(
            buf.Get(), nullptr, this->refBackBufferView.GetAddressOf());
        });

    this->SubscribeSync("Hook/IDXGISwapChain/ResizeBuffers/Pre", [this](auto&&...) {
        this->refBackBufferView = nullptr;
        });

    return true;
}