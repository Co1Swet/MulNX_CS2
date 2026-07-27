#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

class MidTex {
public:
    std::atomic<std::chrono::steady_clock::time_point>* captureTime;
    ComPtr<ID3D11Texture2D> pTex;
    ComPtr<IDXGIKeyedMutex> pMutex;
};

// 环形队列的单个槽位：一对跨设备共享纹理 + 新帧标志 + 捕获时刻(PTS)
struct RingSlot {
    MidTex rawTex;      // 原设备（游戏）上的共享纹理
    MidTex shareTex;    // 录制设备上的共享纹理
    std::atomic<std::chrono::steady_clock::time_point> captureTime;
    static_assert(std::atomic<std::chrono::steady_clock::time_point>::is_always_lock_free, "captureTime must be lock-free");

    RingSlot() {
        this->rawTex.captureTime = &this->captureTime;
        this->shareTex.captureTime = &this->captureTime;
    }
    RingSlot(const RingSlot&) = delete;
    RingSlot& operator=(const RingSlot&) = delete;
    RingSlot(RingSlot&& o) noexcept
        : rawTex(std::move(o.rawTex)), shareTex(std::move(o.shareTex)),
          captureTime(o.captureTime.load(std::memory_order_relaxed)) {}
    RingSlot& operator=(RingSlot&& o) noexcept {
        if (this != &o) {
            rawTex = std::move(o.rawTex);
            shareTex = std::move(o.shareTex);
            captureTime.store(o.captureTime.load(std::memory_order_relaxed), std::memory_order_relaxed);
        }
        return *this;
    }
};

class VCD3D11Manager final : public MediaModuleBase {
    MulNX::GraphicsManager* pGraphicsManager = nullptr;

    moodycamel::BlockingConcurrentQueue<int> forReader{};
    moodycamel::BlockingConcurrentQueue<int> forWriter{};

    void OnPresentFirst(MulNX::Message& msg);
    bool CreateSlot(const D3D11_TEXTURE2D_DESC& desc, RingSlot& slot);

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

    std::optional<int> TryGetReadSide();
    void ReleaseReadSide(int index);

    int GetWriteSide();
    void ReleaseWriteSide(int index);
};