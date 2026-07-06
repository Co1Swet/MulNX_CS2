#pragma once
#include <string>
#include <vector>
#include <cstdint>

enum class EncodeMode { Auto, H264, HEVC };
enum class RateControl { CBR, VBR, CQ };

struct RecordParams {
    // ── 编码器 ──
    EncodeMode  mode        = EncodeMode::Auto;
    RateControl rc          = RateControl::VBR;
    int         bitrate     = 20'000'000;  // bps, CBR/VBR 时有效
    int         cq          = 23;          // CQ 质量 0-51, 越小越好
    int         maxBFrames  = 0;           // 0=无 B 帧
    int         gopSize     = 0;           // 0=自动 = fps×2
    std::string preset      = "p4";        // 编码器预设（nvenc: p1-p7, x264: ultrafast~placebo）
    std::string profile     = "high";      // high / main / baseline

    // ── 捕获 ──
    int  width              = 0;           // 0=原生
    int  height             = 0;           // 0=原生
    int  captureFpsCap      = 60;          // 0=不限制
    int  ringSlots          = 6;

    // ── 运动模糊 ──
    bool enableMotionBlur   = false;       // 全时采样累积式运动模糊（不舍弃多余帧）
    float motionBlurShutter = 1.0f;        // 虚拟快门角度/模糊强度 0.1~1.0
};

struct EncoderCaps {
    std::vector<std::string> hwEncoders;
    std::vector<std::string> swEncoders;
    bool d3d11vaAvailable = false;
};

EncoderCaps DetectEncoderCaps();
