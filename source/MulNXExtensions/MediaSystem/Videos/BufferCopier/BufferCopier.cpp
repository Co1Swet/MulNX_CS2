#include "BufferCopier.hpp"
#include <MulNXExtensions/MediaSystem/Videos/VCD3D11Manager/VCD3D11Manager.hpp>
#include <MulNXExtensions/MediaSystem/MediaParamManager/MediaParamManager.hpp>

bool BufferCopier::Init() {
    this->pVCD3D11Manager = this->FindModule<VCD3D11Manager>("VCD3D11Manager");
    this->pGraphicsManager = this->FindModule<MulNX::GraphicsManager>("GraphicsManager");
    this->pMediaParamManager = this->FindModule<MediaParamManager>("MediaParamManager");

    this->SubscribeSync("MediaSync/PresentCallback", [this](MulNX::Message& msg) {
        this->CopyTexture();
        });
    
    return true;
}

void BufferCopier::CopyTexture() {
    if (!this->shouldCopy.load(std::memory_order_acquire)) return;
    if (!this->pMediaState->recording)return;

    MulNX::Message msgBeforeCopyBackbuffer("MediaSync/BeforeCopyBackbuffer"_hash);
    this->PublishSync(msgBeforeCopyBackbuffer);
    auto&& [info] = msgBeforeCopyBackbuffer.Access<MulNX::VFrameExInfo>();
    if (info.needDrop)return;

    ComPtr<ID3D11Texture2D> backBuffer;
    HRESULT hr = this->pGraphicsManager->pSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (FAILED(hr)) return;

    auto h = this->pVCD3D11Manager->GetWriteSide();
    auto OnExit = scope_exit([&]() {
        this->pVCD3D11Manager->ReleaseWriteSide(h);
        });

    auto& slot = this->pVCD3D11Manager->ring[h].rawTex;

    hr = slot.pMutex->AcquireSync(0, INFINITE);
    if (hr != S_OK) {
        MulNX::ErrorTerminate("锁获取出错");
        return;
    }

    // 获取后备缓冲描述以确定是否 MSAA
    D3D11_TEXTURE2D_DESC bbDescR;
    backBuffer->GetDesc(&bbDescR);

    // 执行拷贝，并在拷贝完成时刻打 PTS
    if (bbDescR.SampleDesc.Count > 1) {
        this->pGraphicsManager->pd3dContext->ResolveSubresource(
            slot.pTex.Get(), 0, backBuffer.Get(), 0,
            this->pVCD3D11Manager->srcDxgiFormat);
    }
    else {
        this->pGraphicsManager->pd3dContext->CopyResource(
            slot.pTex.Get(), backBuffer.Get());
    }

    *slot.pFrameInfo = info;

    hr = slot.pMutex->ReleaseSync(1);
    if (FAILED(hr)) {
        this->LogError("ReleaseSync(1) 失败");
    }
}