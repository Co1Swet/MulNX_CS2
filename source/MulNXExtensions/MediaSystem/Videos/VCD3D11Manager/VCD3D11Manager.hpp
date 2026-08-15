#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

class MidTex {
public:
    MulNX::VFrameExInfo* pFrameInfo = nullptr;
    ComPtr<ID3D11Texture2D> pTex;
    ComPtr<IDXGIKeyedMutex> pMutex;
};

// 环形队列的单个槽位：一对跨设备共享纹理 + 新帧标志 + 捕获时刻(PTS)
struct RingSlot {
    MidTex rawTex;      // 原设备（游戏）上的共享纹理
    MidTex shareTex;    // 录制设备上的共享纹理
    MulNX::VFrameExInfo frameInfo;

    RingSlot() {
        this->rawTex.pFrameInfo = &this->frameInfo;
        this->shareTex.pFrameInfo = &this->frameInfo;
    }
    RingSlot(const RingSlot&) = delete;
    RingSlot& operator=(const RingSlot&) = delete;
    RingSlot(RingSlot&& other) noexcept :
        rawTex(std::move(other.rawTex)),
        shareTex(std::move(other.shareTex)),
        frameInfo(std::move(other.frameInfo)) {}
    
    RingSlot& operator=(RingSlot&& other) noexcept {
        if (this != &other) {
            this->rawTex = std::move(other.rawTex);
            this->shareTex = std::move(other.shareTex);
            this->frameInfo = std::move(other.frameInfo);
        }
        return *this;
    }
};

class VCD3D11Manager final : public MediaModuleBase {
    MulNX::GraphicsManager* pGraphicsManager = nullptr;

    moodycamel::BlockingConcurrentQueue<int> forReader{};
    moodycamel::BlockingConcurrentQueue<int> forWriter{};

    av::PixelFormat DXGIFormatToAvPixelFormat(DXGI_FORMAT format);

    void OnPresentFirst(MulNX::Message& msg);
    bool CreateSlot(const D3D11_TEXTURE2D_DESC& desc, RingSlot& slot);
    void RefreshTextures();

    bool Init() override;
public:
    // 环形队列
    std::vector<RingSlot> ring;

    ComPtr<ID3D11Device> pReadSideDevice;
    ComPtr<ID3D11DeviceContext> pReadSideContext;

    // 源纹理参数（创建环形队列时记录，供编码器配置使用）
    int srcWidth = 0;
    int srcHeight = 0;

    DXGI_FORMAT srcDxgiFormat = DXGI_FORMAT_UNKNOWN;
    av::PixelFormat srcAVFormat = AV_PIX_FMT_NONE;

    std::optional<int> TryGetReadSide();
    void ReleaseReadSide(int index);

    int GetWriteSide();
    void ReleaseWriteSide(int index);
};