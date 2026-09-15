#pragma once
#include <MulNX/MulNX.hpp>
#include "../IVEncoder.hpp"
#include <MulNXUtils/StringCombo.hpp>

class OpenH264Encoder final : public MulNX::Module<OpenH264Encoder>, public MulNX::IVEncoder {
    // 基础编码特性
    StringCombo profile{ { "默认", "constrained_baseline", "main", "high" } };
    StringCombo coder{ { "默认", "cavlc", "cabac" } };
    StringCombo loopfilter{ { "默认", "0", "1" } };

    // 码率控制
    StringCombo rc_mode{ { "默认", "off", "quality", "bitrate", "buffer", "timestamp" } };
    StringCombo allow_skip_frames{ { "默认", "0", "1" } };

    // 切片 / 打包
    int max_nal_size = 0;

    av::Codec codec{};

    void Menu();

    bool Init() override;
    av::Codec* GetAVCodec() override;
    av::Dictionary GetPrivateOpts() override;
};