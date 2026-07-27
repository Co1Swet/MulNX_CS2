#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

class BufferCopier final :public MediaModuleBase {
    class VCD3D11Manager* pVCD3D11Manager = nullptr;
    MulNX::GraphicsManager* pGraphicsManager = nullptr;

    // 可选的捕获帧率上限（0=不限制，全帧捕获）
    std::atomic<int> captureFpsCap{ 0 };

    // 基于时间槽的帧率限制状态（录制启动时由 MediaRecorder 设置）
    std::chrono::steady_clock::time_point recordStartTime;
    int64_t minIntervalUs = 16667;     // captureFpsCap 换算（µs）
    int64_t lastSlot = -1;             // 上次捕获所在时间槽序号

    bool Init()override;

    void CopyTexture();
public:
    void SetCaptureFpsCap(int cap);
    void SetRecordStart(std::chrono::steady_clock::time_point t);
};