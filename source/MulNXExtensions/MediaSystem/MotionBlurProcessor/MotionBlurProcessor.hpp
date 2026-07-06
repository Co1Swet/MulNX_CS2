#pragma once
#include <d3d11.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

// Self-contained GPU motion blur processor (frame accumulation).
// Owns all D3D11 resources for the accumulation pipeline.
// VCD3D11Manager uses this via composition, not inheritance.
class MotionBlurProcessor {
public:
    bool Init(ID3D11Device* device, int width, int height, DXGI_FORMAT backbufferFormat);
    void Release();

    bool IsReady() const { return this->ready; }
    int Width() const { return this->texW; }
    int Height() const { return this->texH; }

    // Accumulate one frame: copy backbuffer to staging, dispatch CS_Accumulate.
    void AccumulateFrame(ID3D11DeviceContext* ctx,
                         ID3D11Texture2D* backBuffer,
                         const D3D11_TEXTURE2D_DESC& bbDesc);

    // Finalize accumulation: dispatch CS_Finalize to output texture.
    // Returns the output texture (ready for CopyResource) and the accumulated frame count.
    // Caller is responsible for CopyResource from OutputTex to ring buffer or target.
    ID3D11Texture2D* TryFinalize(ID3D11DeviceContext* ctx);

    int GetAccumCount() const { return this->accumCount; }

    // Force-reset accumulation without finalizing.
    void ResetAccum(ID3D11DeviceContext* ctx);

private:
    bool ready = false;
    int texW = 0, texH = 0;

    ComPtr<ID3D11Texture2D>             accumTex;     // R16G16B16A16_FLOAT
    ComPtr<ID3D11UnorderedAccessView>   accumUAV;
    ComPtr<ID3D11ShaderResourceView>    accumSRV;
    ComPtr<ID3D11Texture2D>             stagingTex;   // backbuffer copy for CS read
    ComPtr<ID3D11ShaderResourceView>    stagingSRV;
    ComPtr<ID3D11Texture2D>             outputTex;    // finalized output (non-shared, UAV)
    ComPtr<ID3D11UnorderedAccessView>   outputUAV;
    ComPtr<ID3D11ComputeShader>         csAccumulate;
    ComPtr<ID3D11ComputeShader>         csFinalize;
    ComPtr<ID3D11Buffer>                constBuffer;  // holds invCount
    int accumCount = 0;

    ComPtr<ID3DBlob> CompileCS(const char* src, const char* entry);
};
