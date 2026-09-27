#include "ChromaKeyOverlay.hpp"
#include <MulNX/Base/UI/UI.hpp>

bool ChromaKeyOverlay::Menu() {
    MulNX::UI::Checkbox("绿幕", this->enabled);
    MulNX::UI::SliderFloat("绿幕合并阈值", this->m_GreenThreshold, 0, 1);
    return true;
}

bool ChromaKeyOverlay::Init() {
    this->pGraphicsManager = this->FindModule<MulNX::GraphicsManager>("GraphicsManager");
    this->pShaderCompiler = this->FindModule<MulNX::ShaderCompiler>("ShaderCompiler");
    this->UIRegisterCallback("UI.2DVision", [this](auto&&...) {return this->Menu();});

    this->SubscribeSync("Hook/IDXGISwapChain/ResizeBuffers/Pre", [this](auto&&...) {
        this->ReleaseOld();
        });

    this->SubscribeSync("GraphicsSync/D3D11/Init/Post", [this](auto&&...) {
        // 创建绿幕着色器资源
        this->ReleaseOld();
        this->CreateGreenScreenAssets();
        });

    this->SubscribeSync("Hook/ClearDepthStencilView", [this](MulNX::Message& msg) {
        auto&& [pCtx, pDSV, clearFlags] = msg
            .Access<ID3D11DeviceContext*, ID3D11DepthStencilView*, UINT>();
        this->OnClearDepthStencilView(pCtx, pDSV, clearFlags);
        });

    this->SubscribeSync("Hook/Present", [this](auto&&...) {
        // 绿幕渲染
        this->OnPresent();
        });

    return true;
}

void ChromaKeyOverlay::ReleaseOld() {
    this->pDepthSRV = nullptr;
    this->pDepthCopyTex = nullptr;
    this->pColorCopySRV = nullptr;
    this->pColorCopyTex = nullptr;
}

// ------------------------------------------------------------------
// 创建绿幕着色器、常量缓冲区、采样器、混合状态
// ------------------------------------------------------------------
void ChromaKeyOverlay::CreateGreenScreenAssets() {
    auto device = this->pGraphicsManager->pd3dDevice;
    if (!device) return;

    device->CreateVertexShader(this->pShaderCompiler->vsBlob->GetBufferPointer(),
        this->pShaderCompiler->vsBlob->GetBufferSize(), nullptr, &m_pGreenVS);
    device->CreatePixelShader(this->pShaderCompiler->psBlob->GetBufferPointer(),
        this->pShaderCompiler->psBlob->GetBufferSize(), nullptr, &m_pGreenPS);

    D3D11_INPUT_ELEMENT_DESC dummyDesc[] = {
        { "DUMMY", 0, DXGI_FORMAT_R32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    device->CreateInputLayout(dummyDesc, 1, this->pShaderCompiler->vsBlob->GetBufferPointer(), this->pShaderCompiler->vsBlob->GetBufferSize(), &m_pGreenLayout);

    this->pShaderCompiler->Release();

    // 常量缓冲区（存储阈值）
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = 16;  // float4
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    device->CreateBuffer(&cbDesc, nullptr, &m_pGreenCB);

    // 点采样器
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    device->CreateSamplerState(&sampDesc, &m_pPointSampler);

    // 混合状态（可选，保留原写法）
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = FALSE;       // 此处不启用混合，直接覆盖
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    device->CreateBlendState(&blendDesc, &m_pBlendState);

    this->LogSucc("绿幕着色器资源创建成功");
}

void ChromaKeyOverlay::OnClearDepthStencilView(ID3D11DeviceContext* pCtx, ID3D11DepthStencilView* pDSV, UINT ClearFlags) {
    if (!this->enabled.load(std::memory_order_acquire))return;
    if (!(ClearFlags & D3D11_CLEAR_DEPTH))return;
    if (!(pDSV && (UINT_PTR)pDSV > 0x10000))return;
    // 只拷贝，不创建新资源
    if (!this->pDepthCopyTex)return;
    ComPtr<ID3D11Resource> pSrcRes = nullptr;
    pDSV->GetResource(&pSrcRes);
    if (!pSrcRes)return;
    pCtx->CopyResource(this->pDepthCopyTex.Get(), pSrcRes.Get());
}

void ChromaKeyOverlay::OnPresent() {
    if (!this->enabled.load(std::memory_order_acquire))return;
    // 确保颜色/深度副本存在，并拷贝当前帧颜色
    this->EnsureCopyResources();
    this->CopyColorBuffer();
    // 绿幕全屏绘制（覆盖原画面，背景变绿）
    this->RenderGreenScreen();
}

// ------------------------------------------------------------------
// 创建/重建颜色 + 深度副本（在 Present 安全点调用）
// ------------------------------------------------------------------
void ChromaKeyOverlay::EnsureCopyResources() {
    auto pd3dDevice = this->pGraphicsManager->pd3dDevice;
    auto pd3dContext = this->pGraphicsManager->pd3dContext;
    auto pSwapChain = this->pGraphicsManager->pSwapChain;
    if (!pd3dDevice || !pd3dContext) return;

    // ---------- 颜色副本 ----------
    ComPtr<ID3D11Texture2D> backBuffer = nullptr;
    pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    [&]() {
        if (!backBuffer)return;
        D3D11_TEXTURE2D_DESC bbDesc;
        backBuffer->GetDesc(&bbDesc);

        bool colorNeedCreate = (!this->pColorCopyTex || !this->pColorCopySRV ||
            m_ColorWidth != bbDesc.Width ||
            m_ColorHeight != bbDesc.Height ||
            m_ColorCopyFormat != bbDesc.Format);

        if (!colorNeedCreate)return;

        this->pColorCopySRV = nullptr;
        this->pColorCopyTex = nullptr;

        D3D11_TEXTURE2D_DESC copyDesc = {};
        copyDesc.Width = bbDesc.Width;
        copyDesc.Height = bbDesc.Height;
        copyDesc.MipLevels = 1;
        copyDesc.ArraySize = 1;
        copyDesc.Format = bbDesc.Format;
        copyDesc.SampleDesc.Count = 1;
        copyDesc.Usage = D3D11_USAGE_DEFAULT;
        copyDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        if (bbDesc.SampleDesc.Count > 1)
            copyDesc.BindFlags |= D3D11_BIND_RENDER_TARGET; // 支持 MSAA Resolve

        HRESULT hr = pd3dDevice->CreateTexture2D(&copyDesc, nullptr, &this->pColorCopyTex);
        if (!SUCCEEDED(hr))return;
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = bbDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        hr = pd3dDevice->CreateShaderResourceView(
            this->pColorCopyTex.Get(), &srvDesc, &this->pColorCopySRV);
        if (SUCCEEDED(hr)) {
            m_ColorWidth = bbDesc.Width;
            m_ColorHeight = bbDesc.Height;
            m_ColorCopyFormat = bbDesc.Format;
            this->LogSucc("颜色副本创建成功");
        }
        }();
    
    // ---------- 深度副本 ----------
    ComPtr<ID3D11DepthStencilView> curDSV;
    pd3dContext->OMGetRenderTargets(0, nullptr, curDSV.GetAddressOf());
    if (!curDSV) return;

    ComPtr<ID3D11Resource> depthRes;
    curDSV->GetResource(depthRes.GetAddressOf());
    if (!depthRes) return;

    ComPtr<ID3D11Texture2D> depthTex;
    HRESULT hr = depthRes.As(&depthTex);
    if (FAILED(hr) || !depthTex) return;

    D3D11_TEXTURE2D_DESC ddesc;
    depthTex->GetDesc(&ddesc);

    bool depthNeedCreate = (!this->pDepthCopyTex || !this->pDepthSRV ||
        m_DepthWidth != ddesc.Width ||
        m_DepthHeight != ddesc.Height ||
        m_DepthCopyFormat != ddesc.Format);
    if (!depthNeedCreate) return;

    this->pDepthSRV = nullptr;
    this->pDepthCopyTex = nullptr;

    D3D11_TEXTURE2D_DESC copyDesc = {};
    copyDesc.Width = ddesc.Width;
    copyDesc.Height = ddesc.Height;
    copyDesc.MipLevels = 1;
    copyDesc.ArraySize = 1;
    copyDesc.Format = ddesc.Format;           // typeless
    copyDesc.SampleDesc.Count = 1;
    copyDesc.Usage = D3D11_USAGE_DEFAULT;
    copyDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    hr = pd3dDevice->CreateTexture2D(&copyDesc, nullptr, &this->pDepthCopyTex);
    if (!SUCCEEDED(hr))return;
    DXGI_FORMAT srvFmt = DXGI_FORMAT_UNKNOWN;
    switch (ddesc.Format) {
    case DXGI_FORMAT_R24G8_TYPELESS: srvFmt = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; break;
    case DXGI_FORMAT_R32_TYPELESS:   srvFmt = DXGI_FORMAT_R32_FLOAT;              break;
    case DXGI_FORMAT_R16_TYPELESS:   srvFmt = DXGI_FORMAT_R16_UNORM;              break;
    default: srvFmt = ddesc.Format; break;
    }
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = srvFmt;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    hr = pd3dDevice->CreateShaderResourceView(this->pDepthCopyTex.Get(), &srvDesc, &this->pDepthSRV);
    if (SUCCEEDED(hr)) {
        m_DepthWidth = ddesc.Width;
        m_DepthHeight = ddesc.Height;
        m_DepthCopyFormat = ddesc.Format;
        this->LogSucc("深度副本创建成功");
    }
}

// ------------------------------------------------------------------
// 拷贝后备缓冲区 → 颜色副本（非 MSAA 直接 CopyResource）
// ------------------------------------------------------------------
void ChromaKeyOverlay::CopyColorBuffer() {
    auto pd3dContext = this->pGraphicsManager->pd3dContext;
    auto pSwapChain = this->pGraphicsManager->pSwapChain;
    if (!this->pColorCopyTex || !pd3dContext) return;

    ComPtr<ID3D11Texture2D> backBuf = nullptr;
    pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuf);
    if (!backBuf) return;

    D3D11_TEXTURE2D_DESC bbDesc;
    backBuf->GetDesc(&bbDesc);

    if (bbDesc.SampleDesc.Count > 1) {
        pd3dContext->ResolveSubresource(this->pColorCopyTex.Get(), 0, backBuf.Get(), 0, bbDesc.Format);
    }
    else {
        pd3dContext->CopyResource(this->pColorCopyTex.Get(), backBuf.Get());
    }
}

// ------------------------------------------------------------------
// 全屏三角形渲染：深度 < 阈值保留原色，否则绿色
// ------------------------------------------------------------------
void ChromaKeyOverlay::RenderGreenScreen() {
    auto ctx = this->pGraphicsManager->pd3dContext;
    auto view = this->pGraphicsManager->refBackBufferView;

    if (!ctx || !view ||
        !this->pDepthSRV || !this->pColorCopySRV ||
        !m_pGreenVS || !m_pGreenPS)
        return;

    // 保存状态
    ComPtr<ID3D11RenderTargetView> savedRTV = nullptr;
    ComPtr<ID3D11DepthStencilView> savedDSV = nullptr;
    ctx->OMGetRenderTargets(1, &savedRTV, &savedDSV);

    UINT numVP = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    D3D11_VIEWPORT savedVP[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
    ctx->RSGetViewports(&numVP, savedVP);

    ComPtr<ID3D11DepthStencilState> savedDSS = nullptr;
    UINT savedStencilRef = 0;
    ctx->OMGetDepthStencilState(&savedDSS, &savedStencilRef);

    ComPtr<ID3D11BlendState> savedBlend = nullptr;
    FLOAT savedBlendFactor[4] = { 1,1,1,1 };
    UINT savedMask = 0;
    ctx->OMGetBlendState(&savedBlend, savedBlendFactor, &savedMask);

    // 设置管线
    ctx->IASetInputLayout(m_pGreenLayout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);

    ctx->VSSetShader(m_pGreenVS, nullptr, 0);
    ctx->PSSetShader(m_pGreenPS, nullptr, 0);

    ID3D11ShaderResourceView* srvs[2] = { this->pDepthSRV.Get(), this->pColorCopySRV.Get() };
    ctx->PSSetShaderResources(0, 2, srvs);
    ctx->PSSetSamplers(0, 1, &m_pPointSampler);
    ctx->PSSetConstantBuffers(0, 1, &m_pGreenCB);

    // 更新阈值常量缓冲
    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(ctx->Map(m_pGreenCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        memcpy(mapped.pData, &m_GreenThreshold, sizeof(float));
        ctx->Unmap(m_pGreenCB, 0);
    }

    ctx->OMSetRenderTargets(1, view.GetAddressOf(), nullptr);
    D3D11_VIEWPORT vp = { 0.f, 0.f, (FLOAT)m_DepthWidth, (FLOAT)m_DepthHeight, 0.f, 1.f };
    ctx->RSSetViewports(1, &vp);
    ctx->OMSetDepthStencilState(nullptr, 0);
    ctx->OMSetBlendState(m_pBlendState, nullptr, 0xffffffff);

    ctx->Draw(3, 0);   // 全屏三角形

    // 恢复状态
    ctx->OMSetRenderTargets(1, &savedRTV, savedDSV.Get());
    ctx->RSSetViewports(numVP, savedVP);
    ctx->OMSetDepthStencilState(savedDSS.Get(), savedStencilRef);
    ctx->OMSetBlendState(savedBlend.Get(), savedBlendFactor, savedMask);

    ID3D11ShaderResourceView* nullSRV[2] = { nullptr, nullptr };
    ctx->PSSetShaderResources(0, 2, nullSRV);
    ctx->VSSetShader(nullptr, nullptr, 0);
    ctx->PSSetShader(nullptr, nullptr, 0);
}