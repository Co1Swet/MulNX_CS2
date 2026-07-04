#include "VCD3D11Manager.hpp"
#include <MulNXExtensions/MediaSystem/MediaParamManager/MediaParamManager.hpp>

bool VCD3D11Manager::Init() {
    this->pGraphicsManager = this->FindModule<MulNX::GraphicsManager>("GraphicsManager");

    (*this)
        .SubscribeSync("Hook/Present/First", [this](MulNX::Message& msg) {this->OnPresentFirst(msg);})
        .SubscribeSync("Hook/BeforePresent", [this](MulNX::Message& msg) {this->CopyTexture();});
    return true;
}

void VCD3D11Manager::SetCaptureFpsCap(int cap) {
    this->captureFpsCap.store(cap < 0 ? 0 : cap, std::memory_order_release);
    if (cap > 0) this->minIntervalUs = 1'000'000 / cap;
}

void VCD3D11Manager::SetRingCapacity(int n) {
    if (n >= 2) this->ringCapacity = n;
}

void VCD3D11Manager::SetRecordStart(std::chrono::steady_clock::time_point t) {
    this->recordStartTime = t;
    this->lastSlot = -1;
}

bool VCD3D11Manager::CreateSlot(const D3D11_TEXTURE2D_DESC& sharedDesc, RingSlot& slot) {
    HRESULT hr = this->pGraphicsManager->pd3dDevice->CreateTexture2D(&sharedDesc, nullptr, &slot.rawTex.pTex);
    if (FAILED(hr)) {
        this->LogError("环形槽位共享纹理创建失败");
        return false;
    }

    ComPtr<IDXGIResource> pDXGIRes;
    hr = slot.rawTex.pTex.As(&pDXGIRes);
    if (FAILED(hr)) { this->LogError("槽位获取IDXGIResource失败"); return false; }
    HANDLE hSharedHandle = nullptr;
    hr = pDXGIRes->GetSharedHandle(&hSharedHandle);
    if (FAILED(hr)) { this->LogError("槽位获取共享句柄失败"); return false; }

    hr = this->pDevice->OpenSharedResource(hSharedHandle, IID_PPV_ARGS(&slot.shareTex.pTex));
    if (FAILED(hr)) { this->LogError("槽位在捕获设备上打开共享资源失败"); return false; }

    hr = slot.rawTex.pTex.As(&slot.rawTex.pMutex);
    if (FAILED(hr)) { this->LogError("槽位原设备获取KeyedMutex失败"); return false; }
    hr = slot.shareTex.pTex.As(&slot.shareTex.pMutex);
    if (FAILED(hr)) { this->LogError("槽位录制设备获取KeyedMutex失败"); return false; }

    return true;
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
        &this->pDevice, nullptr, &this->pContext
    );
    if (FAILED(hr)) {
        this->LogError("捕获用D3D11设备创建失败");
        return;
    }
    this->LogSucc("捕获用D3D11设备创建成功");

    // 获取原设备后台缓冲区描述
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

    // 共享纹理描述模板：强制非 MSAA、使用后备缓冲的实际格式
    D3D11_TEXTURE2D_DESC sharedDesc = {};
    sharedDesc.Width = bbDesc.Width;
    sharedDesc.Height = bbDesc.Height;
    sharedDesc.MipLevels = 1;
    sharedDesc.ArraySize = 1;
    sharedDesc.Format = rtvDesc.Format;
    sharedDesc.SampleDesc.Count = 1;
    sharedDesc.SampleDesc.Quality = 0;
    sharedDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;
    sharedDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    sharedDesc.CPUAccessFlags = 0;
    sharedDesc.Usage = D3D11_USAGE_DEFAULT;

    // 记录源参数：宽高用后备缓冲，像素格式用 RTV 格式（保证可被 DXGIFormatToAvPixelFormat 识别）
    this->srcWidth = (int)bbDesc.Width;
    this->srcHeight = (int)bbDesc.Height;
    this->srcDxgiFormat = rtvDesc.Format;
    this->LogInfo(std::format("源纹理: {}x{} bbFormat={:#x} rtvFormat={:#x}",
        this->srcWidth, this->srcHeight,
        static_cast<unsigned>(bbDesc.Format),
        static_cast<unsigned>(rtvDesc.Format)));

    // 创建环形队列（容量优先取自 MediaParamManager，未找到则用默认）
    int n = this->ringCapacity;
    auto* params = this->FindModule<MediaParamManager>("MediaParamManager");
    if (params && params->Params().ringSlots >= 2) {
        n = params->Params().ringSlots;
    }
    if (n < 2) n = 6;
    this->ring.clear();
    this->ring.resize(n);
    int created = 0;
    for (int i = 0; i < n; ++i) {
        if (this->CreateSlot(sharedDesc, this->ring[i])) {
            ++created;
        } else {
            this->LogWarning(std::format("环形槽位 {} 创建失败", i));
        }
    }
    if (created < 2) {
        this->LogError("环形队列有效槽位不足(<2)，录制将不可用");
        this->ringReady.store(false);
        return;
    }
    this->ringCapacity = created;
    this->writeIdx.store(0);
    this->readIdx.store(0);
    this->droppedFrames.store(0);
    this->ringReady.store(true, std::memory_order_release);
    this->LogSucc(std::format("共享纹理环形队列就绪，槽位数={}", created));
}

void VCD3D11Manager::CopyTexture() {
    if (!this->runFlag1.load(std::memory_order_acquire)) return;
    if (!this->ringReady.load(std::memory_order_acquire)) return;

    // 基于时间槽的帧率上限：捕获落在当前时间槽的首帧，并量化 PTS 为槽边界
    int cap = this->captureFpsCap.load(std::memory_order_acquire);
    int64_t quantizedPtsUs = -1; // -1 表示不量化，使用实际 now
    if (cap > 0) {
        auto now = std::chrono::steady_clock::now();
        int64_t elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(
            now - this->recordStartTime).count();
        int64_t slot = elapsedUs / this->minIntervalUs;
        if (slot == this->lastSlot) {
            return; // 同一时间槽内不再重复捕获
        }
        this->lastSlot = slot;
        quantizedPtsUs = slot * this->minIntervalUs;
    }

    ComPtr<ID3D11Texture2D> backBuffer;
    HRESULT hr = this->pGraphicsManager->pSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (FAILED(hr)) return;

    int target = this->writeIdx.load(std::memory_order_acquire);
    RingSlot& slot = this->ring[target];

    // 非阻塞获取写锁（key=0 表示可写）；若录制端正在读该槽位则放弃本帧，避免阻塞 Present
    hr = slot.rawTex.pMutex->AcquireSync(0, 0);
    if (hr != S_OK) {
        // WAIT_TIMEOUT = 未拿到锁(录制端在读)，FAILED = 其他错误，均丢弃当前新帧
        this->droppedFrames.fetch_add(1, std::memory_order_relaxed);
        return;
    }

    // 若该槽位仍有未读数据，覆盖即丢弃最旧帧
    if (slot.hasNewFrame.load(std::memory_order_relaxed)) {
        this->droppedFrames.fetch_add(1, std::memory_order_relaxed);
    }

    // 获取后备缓冲描述以确定是否 MSAA
    D3D11_TEXTURE2D_DESC bbDescR;
    backBuffer->GetDesc(&bbDescR);

    // 执行拷贝，并在拷贝完成时刻打 PTS
    if (bbDescR.SampleDesc.Count > 1) {
        this->pGraphicsManager->pd3dContext->ResolveSubresource(
            slot.rawTex.pTex.Get(), 0, backBuffer.Get(), 0,
            static_cast<DXGI_FORMAT>(this->srcDxgiFormat));
    } else {
        this->pGraphicsManager->pd3dContext->CopyResource(
            slot.rawTex.pTex.Get(), backBuffer.Get());
    }
    // 确保 GPU 拷贝命令已提交，否则 ReleaseSync 后读者可能读到未完成的数据（雪花屏）
    this->pGraphicsManager->pd3dContext->Flush();

    // PTS：有帧率上限时量化为时间槽边界，否则取实际 now
    if (quantizedPtsUs >= 0) {
        slot.captureTime.store(this->recordStartTime + std::chrono::microseconds(quantizedPtsUs),
                               std::memory_order_release);
    } else {
        slot.captureTime.store(std::chrono::steady_clock::now(), std::memory_order_release);
    }

    hr = slot.rawTex.pMutex->ReleaseSync(1);
    if (FAILED(hr)) {
        this->LogError("ReleaseSync(1) 失败");
    }
    slot.hasNewFrame.store(true, std::memory_order_release);

    this->writeIdx.store((target + 1) % this->ringCapacity, std::memory_order_release);
}
