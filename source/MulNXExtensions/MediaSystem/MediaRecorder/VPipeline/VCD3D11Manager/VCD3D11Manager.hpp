#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

// 环形队列的单个槽位：一对跨设备共享纹理 + 新帧标志 + 捕获时刻(PTS)
struct RingSlot {
    ComPtr<ID3D11Texture2D> pSourceTex;
    ComPtr<IDXGIKeyedMutex> pSourceMutex;
    ComPtr<ID3D11Texture2D> pCaptureTex;
    ComPtr<IDXGIKeyedMutex> pCaptureMutex;
    MulNX::VFrameExInfo frameInfo{};
};

class VCD3D11Manager final : public MediaModule<VCD3D11Manager> {
    MulNX::GraphicsManager* pGraphicsManager = nullptr;

    std::array<RingSlot, 6> ring;
    moodycamel::BlockingConcurrentQueue<int> forReader{};
    moodycamel::BlockingConcurrentQueue<int> forWriter{};

    av::PixelFormat DXGIFormatToAvPixelFormat(DXGI_FORMAT format);

    void OnPresentFirst(MulNX::Message& msg);
    bool CreateSlot(const D3D11_TEXTURE2D_DESC& desc, RingSlot& slot);
    void RefreshTextures();

    bool Init() override;
    void ReleaseTextures();
public:
    ComPtr<ID3D11Device> pCaptureD3D11Device;
    ComPtr<ID3D11DeviceContext> pCaptureD3D11DeviceContext;

    DXGI_FORMAT srcDxgiFormat = DXGI_FORMAT_UNKNOWN;
    av::PixelFormat srcAVFormat = AV_PIX_FMT_NONE;

    class CaptureTextureView {
        VCD3D11Manager* pVCD3D11Manager = nullptr;
        int index = 0;
    public:
        CaptureTextureView(VCD3D11Manager* pVCD3D11Manager, int index);
        ~CaptureTextureView();
        
        MulNX::VFrameExInfo* pFrameInfo = nullptr;
        ComPtr<ID3D11Texture2D> pTex;
        ComPtr<IDXGIKeyedMutex> pMutex;
    };
    std::optional<CaptureTextureView> TryGetCaptureView();

    class SourceTextureView {
        VCD3D11Manager* pVCD3D11Manager = nullptr;
        int index = 0;
    public:
        SourceTextureView(VCD3D11Manager* pVCD3D11Manager, int index);
        ~SourceTextureView();

        MulNX::VFrameExInfo* pFrameInfo = nullptr;
        ComPtr<ID3D11Texture2D> pTex;
        ComPtr<IDXGIKeyedMutex> pMutex;
    };
    SourceTextureView GetSourceView();
};