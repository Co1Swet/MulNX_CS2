#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>
#include <MulNXExtensions/MediaSystem/MotionBlurProcessor/MotionBlurProcessor.hpp>
#include <atomic>
#include <vector>

class MidTex {
public:
    ComPtr<ID3D11Texture2D> pTex;
    ComPtr<IDXGIKeyedMutex> pMutex;
};

struct RingSlot {
    MidTex rawTex;
    MidTex shareTex;
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

    std::atomic<int> captureFpsCap{ 0 };

    std::chrono::steady_clock::time_point recordStartTime;
    int64_t minIntervalUs = 16667;
    int64_t lastSlot = -1;
    int64_t mbAccumSlot = -1;

    void OnPresentFirst(MulNX::Message& msg);
    void CopyTexture();
    bool CreateSlot(const D3D11_TEXTURE2D_DESC& desc, RingSlot& slot);

    MotionBlurProcessor mbProcessor;

public:
    bool Init() override;

    std::vector<RingSlot> ring;
    int ringCapacity = 6;
    std::atomic<int> writeIdx{ 0 };
    std::atomic<int> readIdx{ 0 };
    std::atomic<uint64_t> droppedFrames{ 0 };
    std::atomic<bool> ringReady{ false };

    ComPtr<ID3D11Device> pDevice;
    ComPtr<ID3D11DeviceContext> pContext;

    int srcWidth = 0;
    int srcHeight = 0;
    DXGI_FORMAT srcDxgiFormat = DXGI_FORMAT_UNKNOWN;

    void SetCaptureFpsCap(int cap);
    void SetRingCapacity(int n);
    void SetRecordStart(std::chrono::steady_clock::time_point t);
};
