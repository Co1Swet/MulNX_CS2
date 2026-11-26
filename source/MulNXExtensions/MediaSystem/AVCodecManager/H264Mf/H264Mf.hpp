#pragma once
#include <MulNXExtensions/MediaSystem/AVCodecManager/AVCodecManager.hpp>
#include <MulNXUtils/StringCombo.hpp>

class H264Mf final : public MediaModule<H264Mf>, public VEncoderMixin<H264Mf> {
    MulReflStr(rate_control, "");
    StringCombo rateControlCombo{ { "cbr", "pc_vbr", "u_vbr", "quality", "ld_vbr", "g_vbr", "gld_vbr" } };
    MulReflStr(scenario, "");
    StringCombo scenarioCombo{ {
        "display_remoting", "video_conference", "archive", "live_streaming",
        "camera_record", "display_remoting_with_feature_map"
    } };
    MulReflInt(quality, -1);
    MulReflBool(hw_encoding, true);
    av::Codec codec{};
    std::atomic<bool> ok = false;

    bool IsAvailable() const override;
    void SettingsMenu() override;
    bool Init() override;
    av::Codec* GetAVCodec() override;
    av::PixelFormat GetPreferredPixelFormat() const override;
    av::Dictionary PrepareOpen(av::VideoEncoderContext* vEncoderContext) const override;
};