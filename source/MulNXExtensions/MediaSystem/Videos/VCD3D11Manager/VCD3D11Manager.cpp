#include "VCD3D11Manager.hpp"

bool VCD3D11Manager::Init() {
    this->pGraphicsManager = this->FindModule<MulNX::GraphicsManager>("GraphicsManager");

    (*this)
        .SubscribeSync("Hook/Present/First", [this](MulNX::Message& msg) {this->OnPresentFirst(msg);})
        .SubscribeSync("Hook/IDXGISwapChain/ResizeBuffers/Pre", [this](MulNX::Message& msg) {this->ReleaseTextures();})
        .SubscribeSync("Hook/IDXGISwapChain/ResizeBuffers/Post", [this](MulNX::Message& msg) {this->RefreshTextures();})
        ;

    return true;
}

av::PixelFormat VCD3D11Manager::DXGIFormatToAvPixelFormat(DXGI_FORMAT format) {
    switch (format) {
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        return AV_PIX_FMT_RGBA;
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        return AV_PIX_FMT_BGRA;
    default:
        return AV_PIX_FMT_NONE;
    }
}

void VCD3D11Manager::OnPresentFirst(MulNX::Message& msg) {
    // 从原设备反推参数，创建兼容录制设备
    D3D_FEATURE_LEVEL originalLevel = this->pGraphicsManager->pd3dDevice->GetFeatureLevel();

    ComPtr<IDXGIDevice> dxgiDevice;
    this->pGraphicsManager->pd3dDevice->QueryInterface(
        __uuidof(IDXGIDevice), (void**)dxgiDevice.GetAddressOf());
    ComPtr<IDXGIAdapter> pAdapter;
    dxgiDevice->GetAdapter(pAdapter.GetAddressOf());

    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    ComPtr<ID3D11Debug> d3dDebug;
    if (SUCCEEDED(this->pGraphicsManager->pd3dDevice->QueryInterface(
        __uuidof(ID3D11Debug), (void**)d3dDebug.GetAddressOf()))) {
        flags |= D3D11_CREATE_DEVICE_DEBUG;
    }

    D3D_DRIVER_TYPE driverType = pAdapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_WARP;

    HRESULT hr = D3D11CreateDevice(
        pAdapter.Get(),
        driverType,
        nullptr,
        flags,
        &originalLevel, 1,
        D3D11_SDK_VERSION,
        &this->pReadSideDevice, nullptr, &this->pReadSideContext
    );
    if (FAILED(hr)) {
        this->LogError("捕获用D3D11设备创建失败");
        return;
    }

    this->LogSucc("捕获用D3D11设备创建成功");
    return this->RefreshTextures();
}

void VCD3D11Manager::RefreshTextures() {
    IDXGISwapChain* pSwapChain = this->pGraphicsManager->pSwapChain;
    if (!pSwapChain) {
        this->LogError("无法获取交换链");
        return;
    }

    DXGI_SWAP_CHAIN_DESC swapDesc;
    HRESULT hr = pSwapChain->GetDesc(&swapDesc);
    if (FAILED(hr)) {
        this->LogError("获取交换链描述失败");
        return;
    }

    // 使用 swapDesc.BufferDesc.Width / Height / Format
    D3D11_TEXTURE2D_DESC sharedDesc = {};
    sharedDesc.Width = swapDesc.BufferDesc.Width;
    sharedDesc.Height = swapDesc.BufferDesc.Height;
    sharedDesc.MipLevels = 1;
    sharedDesc.ArraySize = 1;
    sharedDesc.Format = swapDesc.BufferDesc.Format;
    sharedDesc.SampleDesc.Count = 1;
    sharedDesc.SampleDesc.Quality = 0;
    sharedDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;
    sharedDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    sharedDesc.CPUAccessFlags = 0;
    sharedDesc.Usage = D3D11_USAGE_DEFAULT;

    this->srcDxgiFormat = swapDesc.BufferDesc.Format;
    this->srcAVFormat = this->DXGIFormatToAvPixelFormat(this->srcDxgiFormat);

    this->LogInfo(std::format("纹理: {}x{} Format={:#x}",
        sharedDesc.Width, sharedDesc.Height,
        static_cast<unsigned>(sharedDesc.Format)
    ));

    for (auto& slot : this->ring) {
        if (!this->CreateSlot(sharedDesc, slot)) {
            MulNX::ErrorTerminate("捕获用D3D11纹理槽位创建失败！");
        }
    }

    this->LogSucc(std::format("共享纹理环形队列就绪(共6槽)"));

    this->forWriter.enqueue(0);
    this->forWriter.enqueue(1);
    this->forWriter.enqueue(2);
    this->forWriter.enqueue(3);
    this->forWriter.enqueue(4);
    this->forWriter.enqueue(5);
}

bool VCD3D11Manager::CreateSlot(const D3D11_TEXTURE2D_DESC& sharedDesc, RingSlot& slot) {
    HRESULT hr = this->pGraphicsManager->pd3dDevice->CreateTexture2D(&sharedDesc, nullptr, &slot.rawTex.pTex);
    if (FAILED(hr)) {
        this->LogError("环形槽位共享纹理创建失败");
        return false;
    }

    ComPtr<IDXGIResource> pDXGIRes;
    hr = slot.rawTex.pTex.As(&pDXGIRes);
    if (FAILED(hr)) {
        this->LogError("槽位获取IDXGIResource失败");
        return false;
    }
    HANDLE hSharedHandle = nullptr;
    hr = pDXGIRes->GetSharedHandle(&hSharedHandle);
    if (FAILED(hr)) {
        this->LogError("槽位获取共享句柄失败");
        return false;
    }

    hr = this->pReadSideDevice->OpenSharedResource(hSharedHandle, IID_PPV_ARGS(&slot.shareTex.pTex));
    if (FAILED(hr)) {
        this->LogError("槽位在捕获设备上打开共享资源失败");
        return false;
    }

    hr = slot.rawTex.pTex.As(&slot.rawTex.pMutex);
    if (FAILED(hr)) {
        this->LogError("槽位原设备获取KeyedMutex失败");
        return false;
    }
    hr = slot.shareTex.pTex.As(&slot.shareTex.pMutex);
    if (FAILED(hr)) {
        this->LogError("槽位录制设备获取KeyedMutex失败");
        return false;
    }

    return true;
}
void VCD3D11Manager::ReleaseTextures() {
    int out;
    int count = 0;
    while (count < 6) {
        if (this->forReader.try_dequeue(out))count++;
        if (this->forWriter.try_dequeue(out))count++;
    }
    this->LogInfo("索引队列已清空");
    for (auto& slot : this->ring) {
        slot.rawTex.pMutex.Reset();
        slot.rawTex.pTex.Reset();
        slot.shareTex.pMutex.Reset();
        slot.shareTex.pTex.Reset();
    }
    this->LogInfo("旧资源已释放");
}

std::optional<int> VCD3D11Manager::TryGetReadSide() {
    int out;
    if (!this->forReader.wait_dequeue_timed(out, 200))return std::nullopt;
    else return out;
}
void VCD3D11Manager::ReleaseReadSide(int index) {
    this->forWriter.enqueue(index);
}

int VCD3D11Manager::GetWriteSide() {
    int out;
    this->forWriter.wait_dequeue(out);
    return out;
}
void VCD3D11Manager::ReleaseWriteSide(int index) {
    this->forReader.enqueue(index);
}