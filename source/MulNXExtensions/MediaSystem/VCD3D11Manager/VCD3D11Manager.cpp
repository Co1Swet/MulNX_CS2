#include "VCD3D11Manager.hpp"

bool VCD3D11Manager::Init() {
    this->pGraphicsManager = this->FindModule<MulNX::GraphicsManager>("GraphicsManager");


    (*this)
        .SubscribeSync("Hook/Present/Fisrt", [this](MulNX::Message& msg) {this->OnPresentFirst(msg);})
        .SubscribeSync("Hook/BeforePresent", [this](MulNX::Message& msg) {this->CopyTexture();});
    return true;
}

void VCD3D11Manager::OnPresentFirst(MulNX::Message& msg) {
    // 1. 创建我们的 D3D11 设备（与原设备同一级别）
    D3D_FEATURE_LEVEL originalLevel = this->pGraphicsManager->pd3dDevice->GetFeatureLevel();
    HRESULT hr = D3D11CreateDevice(
        this->pGraphicsManager->D3D11Cfg.pAdapter,
        this->pGraphicsManager->D3D11Cfg.DriverType,
        this->pGraphicsManager->D3D11Cfg.Software,
        this->pGraphicsManager->D3D11Cfg.Flags,
        &originalLevel, 1,
        this->pGraphicsManager->D3D11Cfg.SDKVersion,
        &pDevice, nullptr, &pContext
    );
    if (FAILED(hr)) {
        this->LogError("捕获用D3D11设备创建失败");
        return;
    }
    this->LogSucc("捕获用D3D11设备创建成功");

    // 2. 获取原设备后台缓冲区描述（只需尺寸/格式，不持有资源）
    ComPtr<ID3D11RenderTargetView> pRTV;
    this->pGraphicsManager->pd3dContext->OMGetRenderTargets(1, pRTV.GetAddressOf(), nullptr);
    if (!pRTV) {
        this->LogError("无法获取原设备渲染目标");
        return;
    }

    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
    pRTV->GetDesc(&rtvDesc);
    ComPtr<ID3D11Resource> pBBRes;
    pRTV->GetResource(&pBBRes);
    if (!pBBRes) {
        this->LogError("无法获取后台缓冲区资源");
        return;
    }

    D3D11_TEXTURE2D_DESC bbDesc;
    static_cast<ID3D11Texture2D*>(pBBRes.Get())->GetDesc(&bbDesc);

    // 3. 在原设备上创建共享纹理（格式/尺寸与后台缓冲一致）
    D3D11_TEXTURE2D_DESC sharedDesc = bbDesc;
    sharedDesc.MipLevels = 1;
    sharedDesc.ArraySize = 1;
    sharedDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;
    sharedDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    sharedDesc.CPUAccessFlags = 0;
    sharedDesc.Usage = D3D11_USAGE_DEFAULT;
    sharedDesc.Format = rtvDesc.Format; // 使用RTV的格式，确保兼容性

    hr = this->pGraphicsManager->pd3dDevice->CreateTexture2D(&sharedDesc, nullptr, &this->buffer1.rawTex.pTex);
    if (FAILED(hr)) {
        this->LogError("共享纹理创建失败");
        return;
    }
    this->LogSucc("共享纹理创建成功");

    HANDLE hSharedHandle = nullptr;
    // 4. 获取共享句柄，并在我们自己的设备上打开
    ComPtr<IDXGIResource> pDXGIRes;
    hr = this->buffer1.rawTex.pTex.As(&pDXGIRes);
    if (FAILED(hr)) {
        this->LogError("获取IDXGIResource失败");
        return;
    }
    hr = pDXGIRes->GetSharedHandle(&hSharedHandle);
    if (FAILED(hr)) {
        this->LogError("获取共享句柄失败");
        return;
    }
    this->LogSucc("共享句柄获取成功");

    hr = pDevice->OpenSharedResource(hSharedHandle, IID_PPV_ARGS(&this->buffer1.shareTex.pTex));
    if (FAILED(hr)) {
        this->LogError("在捕获设备上打开共享资源失败");
        return;
    }
    this->LogSucc("共享资源在捕获设备上打开成功");

    // 5. 获取两端的 Keyed Mutex 接口
    hr = this->buffer1.rawTex.pTex.As(&this->buffer1.rawTex.pMutex);
    if (FAILED(hr)) {
        this->LogError("原设备获取KeyedMutex失败");
        return;
    }
    hr = this->buffer1.shareTex.pTex.As(&this->buffer1.shareTex.pMutex);
    if (FAILED(hr)) {
        this->LogError("录制设备获取KeyedMutex失败");
        return;
    }
    this->LogSucc("Keyed Mutex初始化完成");
}

void VCD3D11Manager::CopyTexture() {
    if (!this->runFlag1.load(std::memory_order_acquire)) {
        return;
    }

    auto now = std::chrono::steady_clock::now();
    constexpr std::chrono::microseconds minInterval(16667);
    if (this->lastCapture.has_value() && now - *this->lastCapture < minInterval) {
        return;
    }
    this->lastCapture = now;

    ComPtr<ID3D11Texture2D> backBuffer;
    HRESULT hr = pGraphicsManager->pSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (FAILED(hr)) return;

    // 等待录制端完成上一帧读取（key = 0 表示资源可写）
    hr = this->buffer1.rawTex.pMutex->AcquireSync(0, INFINITE);
    if (FAILED(hr)) {
        this->LogError("AcquireSync(0) 失败");
        return;
    }

    // 执行拷贝
    this->pGraphicsManager->pd3dContext->CopyResource(this->buffer1.rawTex.pTex.Get(), backBuffer.Get());
    this->buffer1.captureTime.store(std::chrono::steady_clock::now());

    // 通知录制端新帧已就绪（key = 1）
    hr = this->buffer1.rawTex.pMutex->ReleaseSync(1);
    if (FAILED(hr)) {
        this->LogError("ReleaseSync(1) 失败");
    }
    this->buffer1.hasNewFrame.store(true, std::memory_order_release);
}