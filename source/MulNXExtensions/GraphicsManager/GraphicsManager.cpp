#include "GraphicsManager.hpp"
#include <MulNX/Base/UI/UI.hpp>

bool MulNX::GraphicsManager::Menu(MulNX::UINode* node) {
    MulNX::UI::Checkbox(I18n("graph.chroma_key.enable").c_str(), this->enabled);
    MulNX::UI::SliderFloat(I18n("graph.chroma_key.threshold").c_str(), this->m_GreenThreshold, 0, 1);
    return true;
}

bool MulNX::GraphicsManager::Init() {
    this->pShaderCompiler = this->Core->ModuleManager()->FindModule<MulNX::ShaderCompiler>("ShaderCompiler");
    this->SendUINode(this->GetName(), [this](MulNX::UINode* node)->bool {return this->Menu(node);});
    return true;
}

void MulNX::GraphicsManager::ReleaseOld() {
    if (this->view) { this->view->Release(); this->view = nullptr; }
    if (m_pDepthSRV) { m_pDepthSRV->Release(); m_pDepthSRV = nullptr; }
    if (m_pDepthCopyTex) { m_pDepthCopyTex->Release(); m_pDepthCopyTex = nullptr; }
    if (m_pColorCopySRV) { m_pColorCopySRV->Release(); m_pColorCopySRV = nullptr; }
    if (m_pColorCopyTex) { m_pColorCopyTex->Release(); m_pColorCopyTex = nullptr; }
    this->needReBuild.store(true, std::memory_order_release);
}

void MulNX::GraphicsManager::BuildNew() {
    if (!this->needReBuild.load(std::memory_order_acquire)) return;
    ID3D11Texture2D* buf = nullptr;
    this->pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&buf);
    if (!buf) return;
    this->pd3dDevice->CreateRenderTargetView(buf, nullptr, &this->view);
    buf->Release();
    this->needReBuild.store(false, std::memory_order_release);
}

void MulNX::GraphicsManager::OnClearDepthStencilView(ID3D11DeviceContext* pCtx, ID3D11DepthStencilView* pDSV, UINT ClearFlags) {
    if (!this->enabled.load(std::memory_order_acquire))return;
    if (!(ClearFlags & D3D11_CLEAR_DEPTH))return;
    if (!(pDSV && (UINT_PTR)pDSV > 0x10000))return;
    // 只拷贝，不创建新资源
    if (!this->m_pDepthCopyTex)return;
    ID3D11Resource* pSrcRes = nullptr;
    pDSV->GetResource(&pSrcRes);
    if (pSrcRes) {
        pCtx->CopyResource(this->m_pDepthCopyTex, pSrcRes);
        pSrcRes->Release();
    }
}

void MulNX::GraphicsManager::OnPresent() {
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
void MulNX::GraphicsManager::EnsureCopyResources() {
    if (!this->pd3dDevice || !this->pd3dContext) return;

    // ---------- 颜色副本 ----------
    ID3D11Texture2D* backBuffer = nullptr;
    this->pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    if (backBuffer) {
        D3D11_TEXTURE2D_DESC bbDesc;
        backBuffer->GetDesc(&bbDesc);

        bool colorNeedCreate = (!m_pColorCopyTex || !m_pColorCopySRV ||
            m_ColorWidth != bbDesc.Width ||
            m_ColorHeight != bbDesc.Height ||
            m_ColorCopyFormat != bbDesc.Format);

        if (colorNeedCreate) {
            if (m_pColorCopySRV) { m_pColorCopySRV->Release(); m_pColorCopySRV = nullptr; }
            if (m_pColorCopyTex) { m_pColorCopyTex->Release(); m_pColorCopyTex = nullptr; }

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

            HRESULT hr = pd3dDevice->CreateTexture2D(&copyDesc, nullptr, &m_pColorCopyTex);
            if (SUCCEEDED(hr)) {
                D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                srvDesc.Format = bbDesc.Format;
                srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MipLevels = 1;
                hr = pd3dDevice->CreateShaderResourceView(m_pColorCopyTex, &srvDesc, &m_pColorCopySRV);
                if (SUCCEEDED(hr)) {
                    m_ColorWidth = bbDesc.Width;
                    m_ColorHeight = bbDesc.Height;
                    m_ColorCopyFormat = bbDesc.Format;
                    this->ISys().LogSucc("颜色副本创建成功");
                }
            }
        }
        backBuffer->Release();
    }

    // ---------- 深度副本 ----------
    ID3D11DepthStencilView* curDSV = nullptr;
    this->pd3dContext->OMGetRenderTargets(0, nullptr, &curDSV);
    if (curDSV) {
        ID3D11Texture2D* depthTex = nullptr;
        curDSV->GetResource(reinterpret_cast<ID3D11Resource**>(&depthTex));
        if (depthTex) {
            D3D11_TEXTURE2D_DESC ddesc;
            depthTex->GetDesc(&ddesc);
            depthTex->Release();

            bool depthNeedCreate = (!m_pDepthCopyTex || !m_pDepthSRV ||
                m_DepthWidth != ddesc.Width ||
                m_DepthHeight != ddesc.Height ||
                m_DepthCopyFormat != ddesc.Format);
            if (depthNeedCreate) {
                if (m_pDepthSRV) { m_pDepthSRV->Release(); m_pDepthSRV = nullptr; }
                if (m_pDepthCopyTex) { m_pDepthCopyTex->Release(); m_pDepthCopyTex = nullptr; }

                D3D11_TEXTURE2D_DESC copyDesc = {};
                copyDesc.Width = ddesc.Width;
                copyDesc.Height = ddesc.Height;
                copyDesc.MipLevels = 1;
                copyDesc.ArraySize = 1;
                copyDesc.Format = ddesc.Format;           // typeless
                copyDesc.SampleDesc.Count = 1;
                copyDesc.Usage = D3D11_USAGE_DEFAULT;
                copyDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

                HRESULT hr = pd3dDevice->CreateTexture2D(&copyDesc, nullptr, &m_pDepthCopyTex);
                if (SUCCEEDED(hr)) {
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
                    hr = pd3dDevice->CreateShaderResourceView(m_pDepthCopyTex, &srvDesc, &m_pDepthSRV);
                    if (SUCCEEDED(hr)) {
                        m_DepthWidth = ddesc.Width;
                        m_DepthHeight = ddesc.Height;
                        m_DepthCopyFormat = ddesc.Format;
                        this->ISys().LogSucc("深度副本创建成功");
                    }
                }
            }
        }
        curDSV->Release();
    }
}

// ------------------------------------------------------------------
// 拷贝后备缓冲区 → 颜色副本（非 MSAA 直接 CopyResource）
// ------------------------------------------------------------------
void MulNX::GraphicsManager::CopyColorBuffer() {
    if (!m_pColorCopyTex || !this->pd3dContext) return;

    ID3D11Texture2D* backBuf = nullptr;
    this->pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuf);
    if (!backBuf) return;

    D3D11_TEXTURE2D_DESC bbDesc;
    backBuf->GetDesc(&bbDesc);

    if (bbDesc.SampleDesc.Count > 1) {
        this->pd3dContext->ResolveSubresource(m_pColorCopyTex, 0, backBuf, 0, bbDesc.Format);
    }
    else {
        this->pd3dContext->CopyResource(m_pColorCopyTex, backBuf);
    }
    backBuf->Release();
}

// ------------------------------------------------------------------
// 创建绿幕着色器、常量缓冲区、采样器、混合状态
// ------------------------------------------------------------------
void MulNX::GraphicsManager::CreateGreenScreenAssets() {
    auto device = this->pd3dDevice;
    if (!device) return;

    device->CreateVertexShader(this->pShaderCompiler->vsBlob->GetBufferPointer(),
        this->pShaderCompiler->vsBlob->GetBufferSize(), nullptr, &m_pGreenVS);
    device->CreatePixelShader(this->pShaderCompiler->psBlob->GetBufferPointer(),
        this->pShaderCompiler->psBlob->GetBufferSize(), nullptr, &m_pGreenPS);

    D3D11_INPUT_ELEMENT_DESC dummyDesc[] = {
        {"DUMMY", 0, DXGI_FORMAT_R32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
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

    this->ISys().LogSucc("绿幕着色器资源创建成功");
}

// ------------------------------------------------------------------
// 全屏三角形渲染：深度 < 阈值保留原色，否则绿色
// ------------------------------------------------------------------
void MulNX::GraphicsManager::RenderGreenScreen() {
    if (!this->pd3dContext || !this->view ||
        !m_pDepthSRV || !m_pColorCopySRV ||
        !m_pGreenVS || !m_pGreenPS)
        return;

    auto ctx = this->pd3dContext;

    // 保存状态
    ID3D11RenderTargetView* savedRTV = nullptr;
    ID3D11DepthStencilView* savedDSV = nullptr;
    ctx->OMGetRenderTargets(1, &savedRTV, &savedDSV);

    UINT numVP = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    D3D11_VIEWPORT savedVP[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
    ctx->RSGetViewports(&numVP, savedVP);

    ID3D11DepthStencilState* savedDSS = nullptr;
    UINT savedStencilRef = 0;
    ctx->OMGetDepthStencilState(&savedDSS, &savedStencilRef);

    ID3D11BlendState* savedBlend = nullptr;
    FLOAT savedBlendFactor[4] = { 1,1,1,1 };
    UINT savedMask = 0;
    ctx->OMGetBlendState(&savedBlend, savedBlendFactor, &savedMask);

    // 设置管线
    ctx->IASetInputLayout(m_pGreenLayout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);

    ctx->VSSetShader(m_pGreenVS, nullptr, 0);
    ctx->PSSetShader(m_pGreenPS, nullptr, 0);

    ID3D11ShaderResourceView* srvs[2] = { m_pDepthSRV, m_pColorCopySRV };
    ctx->PSSetShaderResources(0, 2, srvs);
    ctx->PSSetSamplers(0, 1, &m_pPointSampler);
    ctx->PSSetConstantBuffers(0, 1, &m_pGreenCB);

    // 更新阈值常量缓冲
    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(ctx->Map(m_pGreenCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        memcpy(mapped.pData, &m_GreenThreshold, sizeof(float));
        ctx->Unmap(m_pGreenCB, 0);
    }

    ctx->OMSetRenderTargets(1, &this->view, nullptr);
    D3D11_VIEWPORT vp = { 0.f, 0.f, (FLOAT)m_DepthWidth, (FLOAT)m_DepthHeight, 0.f, 1.f };
    ctx->RSSetViewports(1, &vp);
    ctx->OMSetDepthStencilState(nullptr, 0);
    ctx->OMSetBlendState(m_pBlendState, nullptr, 0xffffffff);

    ctx->Draw(3, 0);   // 全屏三角形

    // 恢复状态
    ctx->OMSetRenderTargets(1, &savedRTV, savedDSV);
    ctx->RSSetViewports(numVP, savedVP);
    ctx->OMSetDepthStencilState(savedDSS, savedStencilRef);
    ctx->OMSetBlendState(savedBlend, savedBlendFactor, savedMask);

    ID3D11ShaderResourceView* nullSRV[2] = { nullptr, nullptr };
    ctx->PSSetShaderResources(0, 2, nullSRV);
    ctx->VSSetShader(nullptr, nullptr, 0);
    ctx->PSSetShader(nullptr, nullptr, 0);

    if (savedRTV) savedRTV->Release();
    if (savedDSV) savedDSV->Release();
    if (savedDSS) savedDSS->Release();
    if (savedBlend) savedBlend->Release();
}