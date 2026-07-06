#pragma once
#include <d3d11.h>

// Minimal CS binding guard: saves/restores exactly 1 compute shader,
// 1 SRV at slot 0, 1 UAV at slot 0, and 1 constant buffer at slot 0.
// This is all MotionBlurProcessor needs. Compared to a full state
// snapshot (hundreds of D3D11 API calls), this does ~8 API calls total.
class CSBindingGuard {
    ID3D11DeviceContext* ctx;
    ID3D11ComputeShader* cs = nullptr;
    ID3D11ShaderResourceView* srv = nullptr;
    ID3D11UnorderedAccessView* uav = nullptr;
    ID3D11Buffer* cb = nullptr;
    UINT uavCounter = 0;

public:
    CSBindingGuard(ID3D11DeviceContext* context) : ctx(context) {
        if (!this->ctx) return;
        this->ctx->CSGetShader(&this->cs, nullptr, nullptr);
        this->ctx->CSGetShaderResources(0, 1, &this->srv);
        this->ctx->CSGetUnorderedAccessViews(0, 1, &this->uav);
        this->ctx->CSGetConstantBuffers(0, 1, &this->cb);
    }

    ~CSBindingGuard() {
        if (!this->ctx) return;
        this->ctx->CSSetShader(this->cs, nullptr, 0);
        this->ctx->CSSetShaderResources(0, 1, &this->srv);
        this->ctx->CSSetUnorderedAccessViews(0, 1, &this->uav, &this->uavCounter);
        if (this->cb) this->ctx->CSSetConstantBuffers(0, 1, &this->cb);

        if (this->cs) this->cs->Release();
        if (this->srv) this->srv->Release();
        if (this->uav) this->uav->Release();
        if (this->cb) this->cb->Release();
    }

    CSBindingGuard(const CSBindingGuard&) = delete;
    CSBindingGuard& operator=(const CSBindingGuard&) = delete;
};
