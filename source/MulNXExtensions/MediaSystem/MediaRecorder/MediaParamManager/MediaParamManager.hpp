#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

class MediaParamManager :public MediaModule<MediaParamManager> {
    void Menu();
    bool Init()override;
public:
    MulReflInt(bitrate, 80'000'000);  // bps, CBR/VBR 时有效
    MulReflInt(maxBFrames, 0);           // 0=无 B 帧
    MulReflInt(gopSize, 0);           // 0=自动 = fps×2
    MulReflInt(width, 0);           // 0=原生
    MulReflInt(height, 0);           // 0=原生
    MulReflInt(targetFPS, 30);          // 0=不限制
};