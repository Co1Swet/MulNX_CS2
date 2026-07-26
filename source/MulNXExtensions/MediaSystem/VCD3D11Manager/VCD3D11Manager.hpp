#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

class MidTex {
public:
    ComPtr<ID3D11Texture2D> pTex;
    ComPtr<IDXGIKeyedMutex> pMutex;
};

// 环形队列的单个槽位：一对跨设备共享纹理 + 新帧标志 + 捕获时刻(PTS)
struct RingSlot {
    MidTex rawTex;      // 原设备（游戏）上的共享纹理
    MidTex shareTex;    // 录制设备上的共享纹理
    std::atomic<bool> hasNewFrame{ false };
    std::atomic<std::chrono::steady_clock::time_point> captureTime;
    static_assert(std::atomic<std::chrono::steady_clock::time_point>::is_always_lock_free, "captureTime must be lock-free");

    RingSlot() noexcept = default;
    RingSlot(const RingSlot&) = delete;
    RingSlot& operator=(const RingSlot&) = delete;
    RingSlot(RingSlot&& o) noexcept
        : rawTex(std::move(o.rawTex)), shareTex(std::move(o.shareTex)),
          hasNewFrame(o.hasNewFrame.load(std::memory_order_relaxed)),
          captureTime(o.captureTime.load(std::memory_order_relaxed)) {}
    RingSlot& operator=(RingSlot&& o) noexcept {
        if (this != &o) {
            rawTex = std::move(o.rawTex);
            shareTex = std::move(o.shareTex);
            hasNewFrame.store(o.hasNewFrame.load(std::memory_order_relaxed), std::memory_order_relaxed);
            captureTime.store(o.captureTime.load(std::memory_order_relaxed), std::memory_order_relaxed);
        }
        return *this;
    }
};

class VCD3D11Manager final : public MediaModuleBase {
    MulNX::GraphicsManager* pGraphicsManager = nullptr;

    // 可选的捕获帧率上限（0=不限制，全帧捕获）
    std::atomic<int> captureFpsCap{ 0 };

    // 基于时间槽的帧率限制状态（录制启动时由 MediaRecorder 设置）
    std::chrono::steady_clock::time_point recordStartTime;
    int64_t minIntervalUs = 16667;     // captureFpsCap 换算（µs）
    int64_t lastSlot = -1;             // 上次捕获所在时间槽序号

    void OnPresentFirst(MulNX::Message& msg);
    void CopyTexture();
    bool CreateSlot(const D3D11_TEXTURE2D_DESC& desc, RingSlot& slot);

public:
    bool Init() override;

    // 环形队列
    std::vector<RingSlot> ring;
    int ringCapacity = 6;
    std::atomic<int> writeIdx{ 0 };
    std::atomic<int> readIdx{ 0 };
    std::atomic<uint64_t> droppedFrames{ 0 };
    std::atomic<bool> ringReady{ false };
    // runFlag1 沿用基类 ModuleComponents::runFlag1，由 VideoCapturer 置位以启用拷贝

    ComPtr<ID3D11Device> pDevice;
    ComPtr<ID3D11DeviceContext> pContext;

    // 源纹理参数（创建环形队列时记录，供编码器配置使用）
    int srcWidth = 0;
    int srcHeight = 0;
    DXGI_FORMAT srcDxgiFormat = DXGI_FORMAT_UNKNOWN;

    void SetCaptureFpsCap(int cap);
    void SetRecordStart(std::chrono::steady_clock::time_point t);
};
