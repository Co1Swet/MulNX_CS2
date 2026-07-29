#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

enum class EncodeMode { Auto, H264, HEVC };
enum class RateControl { CBR, VBR, CQ };

class MediaParamManager :public MediaModuleBase {
    bool Init()override;
public:
    // ── 编码器 ──
    EncodeMode  mode = EncodeMode::Auto;
    RateControl rc = RateControl::VBR;
    int         bitrate = 20'000'000;  // bps, CBR/VBR 时有效
    int         cq = 23;          // CQ 质量 0-51, 越小越好
    int         maxBFrames = 0;           // 0=无 B 帧
    int         gopSize = 0;           // 0=自动 = fps×2
    std::string preset = "p4";        // 编码器预设（nvenc: p1-p7, x264: ultrafast~placebo）
    std::string profile = "high";      // high / main / baseline

    
    int  width = 0;           // 0=原生
    int  height = 0;           // 0=原生
    std::atomic<int> targetFPS = 60;          // 0=不限制
};
