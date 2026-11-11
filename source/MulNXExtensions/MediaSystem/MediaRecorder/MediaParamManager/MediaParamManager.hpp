#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

enum class RateControl { CBR, VBR, CQ };

class MediaParamManager :public MediaModuleBase {
    void Menu();
    bool Init()override;
public:
    // 编码器普通设置
    RateControl rc = RateControl::CBR;
    MulReflInt(bitrate, 80'000'000);  // bps, CBR/VBR 时有效
    int  cq = 23;          // CQ 质量 0-51, 越小越好
    int  maxBFrames = 0;           // 0=无 B 帧
    int  gopSize = 0;           // 0=自动 = fps×2
    int  width = 0;           // 0=原生
    int  height = 0;           // 0=原生
    MulReflInt(targetFPS, 30);          // 0=不限制
};
