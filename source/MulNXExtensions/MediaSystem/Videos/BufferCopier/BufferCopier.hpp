#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

class BufferCopier final :public MediaModuleBase {
    class VCD3D11Manager* pVCD3D11Manager = nullptr;
    class MediaParamManager* pMediaParamManager = nullptr;
    MulNX::GraphicsManager* pGraphicsManager = nullptr;

    // 基于时间槽的帧率限制状态（录制启动时由 MediaRecorder 设置）
    std::atomic<std::chrono::steady_clock::time_point> recordStartTime;
    std::atomic<int64_t> minIntervalUs = 16667;     // captureFpsCap 换算（µs）
    std::atomic<int64_t> lastSlot = -1;             // 上次捕获所在时间槽序号

    bool Init()override;
    void CopyTexture();
    void SetRecordStart(std::chrono::steady_clock::time_point t);
public:
    std::atomic<bool> shouldCopy = false;
};