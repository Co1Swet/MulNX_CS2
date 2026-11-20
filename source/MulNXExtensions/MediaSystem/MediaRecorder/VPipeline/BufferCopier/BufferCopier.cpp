#include "BufferCopier.hpp"
#include <VPipeline/VCD3D11Manager/VCD3D11Manager.hpp>

bool BufferCopier::Init() {
    this->pVCD3D11Manager = this->FindModule<VCD3D11Manager>();
    this->pGraphicsManager = this->FindModule<MulNX::GraphicsManager>();

    this->SubscribeSync("MediaSync/PresentCallback", [this](MulNX::Message& msg) {
        this->CopyTexture();
        });

    this->SubscribeSync("MediaSync/SetOn", [this](MulNX::Message& msg) {
        this->shouldCopy.store(true);
        });

    this->SubscribeSync("MediaSync/SetOff", [this](MulNX::Message& msg) {
        this->shouldCopy.store(false);
        });

    return true;
}

void BufferCopier::CopyTexture() {
    if (!this->shouldCopy.load()) return;

    MulNX::Message msgBeforeCopyBackbuffer("MediaSync/BeforeCopyBackbuffer"_hash);
    auto&& [info] = msgBeforeCopyBackbuffer.Access<MulNX::VFrameExInfo>();
    this->PublishSync(msgBeforeCopyBackbuffer);
    if (info.needDrop)return;

    ComPtr<ID3D11Texture2D> backBuffer;
    HRESULT hr = this->pGraphicsManager->pSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (FAILED(hr)) return;

    auto view = this->pVCD3D11Manager->GetSourceView();

    hr = view.pMutex->AcquireSync(0, INFINITE);
    if (hr != S_OK)MulNX::ErrorTerminate("锁获取出错");
    auto rel = scope_exit([&]() {
        if (FAILED(view.pMutex->ReleaseSync(1))) {
            MulNX::ErrorTerminate("ReleaseSync(1) 失败");
        }});

    // 获取后备缓冲描述以确定是否 MSAA
    D3D11_TEXTURE2D_DESC bbDescR;
    backBuffer->GetDesc(&bbDescR);

    // 执行拷贝
    if (bbDescR.SampleDesc.Count > 1) {
        this->pGraphicsManager->pd3dContext->ResolveSubresource(
            view.pTex.Get(), 0, backBuffer.Get(), 0,
            this->pVCD3D11Manager->srcDxgiFormat);
    }
    else {
        this->pGraphicsManager->pd3dContext->CopyResource(
            view.pTex.Get(), backBuffer.Get());
    }
    *view.pFrameInfo = info;
}