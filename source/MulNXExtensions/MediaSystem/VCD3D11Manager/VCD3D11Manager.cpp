#include "VCD3D11Manager.hpp"

bool VCD3D11Manager::Init() {
    this->ISys()
        .SubscribeSync("Hook/Present/Fisrt", [this](MulNX::Message& msg) {this->OnPresentFirst(msg);});
    
    return true;
}

void VCD3D11Manager::OnPresentFirst(MulNX::Message& msg) {
    // 1. 创建我们的 D3D11 设备（与原设备同一级别）
    D3D_FEATURE_LEVEL originalLevel = pGraphicsManager->pd3dDevice->GetFeatureLevel();
    D3D11CreateDevice(
        pGraphicsManager->D3D11Cfg.pAdapter,
        pGraphicsManager->D3D11Cfg.DriverType,
        pGraphicsManager->D3D11Cfg.Software,
        pGraphicsManager->D3D11Cfg.Flags,
        &originalLevel, 1,
        pGraphicsManager->D3D11Cfg.SDKVersion,
        &this->pDevice, nullptr, &this->pContext
    );

    // 2. 获取原设备后台缓冲区描述（只需尺寸/格式，不持有资源）
    ID3D11DeviceContext* origCtx = pGraphicsManager->pd3dContext;
    ID3D11RenderTargetView* pRTV = nullptr;
    origCtx->OMGetRenderTargets(1, &pRTV, nullptr);
    if (!pRTV) return;

    ID3D11Resource* pBBRes = nullptr;
    pRTV->GetResource(&pBBRes);
    pRTV->Release();

    D3D11_TEXTURE2D_DESC bbDesc;
    static_cast<ID3D11Texture2D*>(pBBRes)->GetDesc(&bbDesc);
    pBBRes->Release();

    // 3. 在原设备上创建共享纹理（格式/尺寸与后台缓冲一致）
    D3D11_TEXTURE2D_DESC sharedDesc = bbDesc;
    sharedDesc.MipLevels = 1;
    sharedDesc.ArraySize = 1;
    sharedDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;   // 关键
    sharedDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;   // 或按需
    sharedDesc.CPUAccessFlags = 0;
    sharedDesc.Usage = D3D11_USAGE_DEFAULT;

    ID3D11Device* origDevice = pGraphicsManager->pd3dDevice;
    origDevice->CreateTexture2D(&sharedDesc, nullptr, &pSharedTex);

    // 4. 获取共享句柄，并在我们自己的设备上打开
    IDXGIResource* pDXGIRes = nullptr;
    pSharedTex->QueryInterface(IID_PPV_ARGS(&pDXGIRes));
    pDXGIRes->GetSharedHandle(&hSharedHandle);
    pDXGIRes->Release();

    // pSharedTex 由原设备持有，后续每帧拷贝后会再次共享（无需每次打开）
    // 我们设备打开一次即可（除非缓冲描述变化，基本不会）
    this->pDevice->OpenSharedResource(hSharedHandle, IID_PPV_ARGS(&pRemoteTex));
}