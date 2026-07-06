#include "MotionBlurProcessor.hpp"
#include "CSBindingGuard.hpp"
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

// ── Shader sources ──

static const char* g_CSAccumulateSrc = R"(
Texture2D<float4>        SrcTex   : register(t0);
RWTexture2D<float4>      AccumTex : register(u0);
[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID) {
    float4 c = SrcTex.Load(int3(DTid.xy, 0));
    c.rgb = pow(max(c.rgb, 0.0), 2.2);   // sRGB -> Linear
    AccumTex[DTid.xy] += c;
}
)";

static const char* g_CSFinalizeSrc = R"(
Texture2D<float4>        AccumTex : register(t0);
RWTexture2D<float4>      OutTex   : register(u0);
cbuffer CB : register(b0) {
    float g_InvCount;
    float3 g_Pad;
};
[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID) {
    float4 c = AccumTex.Load(int3(DTid.xy, 0));
    c.rgb *= g_InvCount;                 // average
    c.rgb = pow(max(c.rgb, 0.0), 1.0 / 2.2); // Linear -> sRGB
    OutTex[DTid.xy] = c;
}
)";

static DXGI_FORMAT DeSRGB(DXGI_FORMAT f) {
    switch (f) {
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return DXGI_FORMAT_R8G8B8A8_UNORM;
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return DXGI_FORMAT_B8G8R8A8_UNORM;
    default: return f;
    }
}

// ── Implementation ──

ComPtr<ID3DBlob> MotionBlurProcessor::CompileCS(const char* src, const char* entry) {
    ComPtr<ID3DBlob> blob, errBlob;
    HRESULT hr = D3DCompile(src, strlen(src), nullptr, nullptr, nullptr,
        entry, "cs_5_0", 0, 0, &blob, &errBlob);
    if (FAILED(hr)) {
        if (errBlob) OutputDebugStringA((char*)errBlob->GetBufferPointer());
        return nullptr;
    }
    return blob;
}

bool MotionBlurProcessor::Init(ID3D11Device* device, int width, int height, DXGI_FORMAT backbufferFormat) {
    if (this->ready) return true;
    if (!device || width <= 0 || height <= 0 || backbufferFormat == DXGI_FORMAT_UNKNOWN) return false;

    DXGI_FORMAT unormFmt = DeSRGB(backbufferFormat);
    UINT w = (UINT)width, h = (UINT)height;

    // 1. Accumulation texture: R16G16B16A16_FLOAT, UAV + SRV
    D3D11_TEXTURE2D_DESC accumDesc = {};
    accumDesc.Width = w; accumDesc.Height = h;
    accumDesc.MipLevels = 1; accumDesc.ArraySize = 1;
    accumDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    accumDesc.SampleDesc.Count = 1;
    accumDesc.Usage = D3D11_USAGE_DEFAULT;
    accumDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    if (FAILED(device->CreateTexture2D(&accumDesc, nullptr, &this->accumTex))) return false;

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = accumDesc.Format;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice = 0;
    if (FAILED(device->CreateUnorderedAccessView(this->accumTex.Get(), &uavDesc, &this->accumUAV))) return false;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = accumDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    if (FAILED(device->CreateShaderResourceView(this->accumTex.Get(), &srvDesc, &this->accumSRV))) return false;

    // 2. Staging texture: non-sRGB UNORM, SRV only (CS reads raw sRGB bytes)
    D3D11_TEXTURE2D_DESC stgDesc = accumDesc;
    stgDesc.Format = unormFmt;
    stgDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    if (FAILED(device->CreateTexture2D(&stgDesc, nullptr, &this->stagingTex))) return false;

    D3D11_SHADER_RESOURCE_VIEW_DESC stgSrv = {};
    stgSrv.Format = unormFmt;
    stgSrv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    stgSrv.Texture2D.MipLevels = 1;
    if (FAILED(device->CreateShaderResourceView(this->stagingTex.Get(), &stgSrv, &this->stagingSRV))) return false;

    // 3. Output texture: UAV writable, non-shared
    D3D11_TEXTURE2D_DESC outDesc = stgDesc;
    outDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    if (FAILED(device->CreateTexture2D(&outDesc, nullptr, &this->outputTex))) return false;

    D3D11_UNORDERED_ACCESS_VIEW_DESC outUav = {};
    outUav.Format = unormFmt;
    outUav.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
    outUav.Texture2D.MipSlice = 0;
    if (FAILED(device->CreateUnorderedAccessView(this->outputTex.Get(), &outUav, &this->outputUAV))) return false;

    // 4. Constant buffer: 16 bytes (float invCount + float[3] pad)
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = 16;
    cbDesc.Usage = D3D11_USAGE_DEFAULT;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    if (FAILED(device->CreateBuffer(&cbDesc, nullptr, &this->constBuffer))) return false;

    // 5. Compute shaders
    auto accumBlob = this->CompileCS(g_CSAccumulateSrc, "main");
    auto finalBlob = this->CompileCS(g_CSFinalizeSrc, "main");
    if (!accumBlob || !finalBlob) return false;
    if (FAILED(device->CreateComputeShader(accumBlob->GetBufferPointer(), accumBlob->GetBufferSize(), nullptr, &this->csAccumulate))) return false;
    if (FAILED(device->CreateComputeShader(finalBlob->GetBufferPointer(), finalBlob->GetBufferSize(), nullptr, &this->csFinalize))) return false;

    this->texW = (int)w; this->texH = (int)h;
    this->accumCount = 0;
    this->ready = true;
    return true;
}

void MotionBlurProcessor::Release() {
    this->accumTex.Reset(); this->accumUAV.Reset(); this->accumSRV.Reset();
    this->stagingTex.Reset(); this->stagingSRV.Reset();
    this->outputTex.Reset(); this->outputUAV.Reset();
    this->csAccumulate.Reset(); this->csFinalize.Reset();
    this->constBuffer.Reset();
    this->accumCount = 0; this->texW = this->texH = 0;
    this->ready = false;
}

void MotionBlurProcessor::AccumulateFrame(ID3D11DeviceContext* ctx,
                                          ID3D11Texture2D* backBuffer,
                                          const D3D11_TEXTURE2D_DESC& bbDesc) {
    if (!ctx || !this->ready) return;

    // Copy/resolve backbuffer -> staging (backbuffer typically has no SRV bind)
    if (bbDesc.SampleDesc.Count > 1) {
        ctx->ResolveSubresource(this->stagingTex.Get(), 0, backBuffer, 0, DeSRGB(bbDesc.Format));
    } else {
        ctx->CopyResource(this->stagingTex.Get(), backBuffer);
    }

    // Save/restore only the CS slots we touch
    CSBindingGuard guard(ctx);
    ID3D11ShaderResourceView* srvs[] = { this->stagingSRV.Get() };
    ID3D11UnorderedAccessView* uavs[] = { this->accumUAV.Get() };
    UINT uavCounts[] = { 0 };
    ctx->CSSetShader(this->csAccumulate.Get(), nullptr, 0);
    ctx->CSSetShaderResources(0, 1, srvs);
    ctx->CSSetUnorderedAccessViews(0, 1, uavs, uavCounts);
    ctx->CSSetConstantBuffers(0, 0, nullptr);
    UINT gx = ((UINT)this->texW + 7) / 8, gy = ((UINT)this->texH + 7) / 8;
    ctx->Dispatch(gx, gy, 1);
    ++this->accumCount;
}

ID3D11Texture2D* MotionBlurProcessor::TryFinalize(ID3D11DeviceContext* ctx) {
    if (!ctx || !this->ready || this->accumCount <= 0) return nullptr;

    // Update constant buffer: invCount
    struct { float invCount; float pad[3]; } cbData;
    cbData.invCount = 1.0f / static_cast<float>(this->accumCount);
    cbData.pad[0] = cbData.pad[1] = cbData.pad[2] = 0.f;
    ctx->UpdateSubresource(this->constBuffer.Get(), 0, nullptr, &cbData, 0, 0);

    // Dispatch CS_Finalize: accumSRV -> outputUAV
    {
        CSBindingGuard guard(ctx);
        ID3D11ShaderResourceView* srvs[] = { this->accumSRV.Get() };
        ID3D11UnorderedAccessView* uavs[] = { this->outputUAV.Get() };
        UINT uavCounts[] = { 0 };
        ID3D11Buffer* cbs[] = { this->constBuffer.Get() };
        ctx->CSSetShader(this->csFinalize.Get(), nullptr, 0);
        ctx->CSSetShaderResources(0, 1, srvs);
        ctx->CSSetUnorderedAccessViews(0, 1, uavs, uavCounts);
        ctx->CSSetConstantBuffers(0, 1, cbs);
        UINT gx = ((UINT)this->texW + 7) / 8, gy = ((UINT)this->texH + 7) / 8;
        ctx->Dispatch(gx, gy, 1);
    }

    // Don't reset accumCount here — caller decides when to reset via ResetAccum()
    return this->outputTex.Get();
}

void MotionBlurProcessor::ResetAccum(ID3D11DeviceContext* ctx) {
    if (!ctx || !this->ready) return;
    float clear[4] = { 0.f, 0.f, 0.f, 0.f };
    ctx->ClearUnorderedAccessViewFloat(this->accumUAV.Get(), clear);
    this->accumCount = 0;
}
