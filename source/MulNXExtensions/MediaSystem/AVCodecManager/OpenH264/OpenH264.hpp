#pragma once
#include <MulNXExtensions/MediaSystem/AVCodecManager/AVCodecManager.hpp>
#include <MulNXUtils/StringCombo.hpp>

class OpenH264Encoder final : public MulNX::Module<OpenH264Encoder>, public VEncoderMixin<OpenH264Encoder> {
    // 基础编码特性
    MulReflStr(profile, "");
    StringCombo profileCombo{ { "constrained_baseline", "main", "high" } };
    MulReflStr(coder, "");
    StringCombo coderCombo{ { "cavlc", "cabac" } };
    MulReflStr(loopfilter, "");
    StringCombo loopfilterCombo{ { "0", "1" } };

    // 码率控制
    MulReflStr(rc_mode, "");
    StringCombo rc_modeCombo{ { "off", "quality", "bitrate", "buffer", "timestamp" } };
    MulReflStr(allow_skip_frames, "");
    StringCombo allow_skip_framesCombo{ { "0", "1" } };

    // 切片 / 打包
    MulReflInt(max_nal_size, 0);

    inline bool SupportsConstantQuality()const override { return false; }

    av::Codec codec{};

    bool IsAvailable()const override;
    void SettingsMenu()override;
    bool Init()override;
    av::Codec* GetAVCodec()override;
    av::Dictionary GetPrivateOpts()const override;
};