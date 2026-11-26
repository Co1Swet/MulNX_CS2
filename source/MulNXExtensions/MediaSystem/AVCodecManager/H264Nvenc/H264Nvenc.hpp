#pragma once
#include <MulNXExtensions/MediaSystem/AVCodecManager/AVCodecManager.hpp>
#include <MulNXUtils/StringCombo.hpp>

class H264Nvenc final : public MediaModule<H264Nvenc>, public VEncoderMixin<H264Nvenc> {
    StringCombo rcCombo{ { "cbr", "vbr", "constqp" } };
    MulReflStr(rc, "constqp");
    MulReflInt(preset, 3);
    MulReflInt(qp, 20);
    MulReflInt(qmax, 25);
    MulReflInt(lookahead, 0);
    MulReflInt(surfaces, 4);
    av::Codec codec{};
    std::atomic<bool> ok = false;

    bool IsAvailable() const override;
    void SettingsMenu() override;
    bool Init() override;
    av::Codec* GetAVCodec() override;
    av::PixelFormat GetPreferredPixelFormat()const override;
    av::Dictionary PrepareOpen(av::VideoEncoderContext* vEncoderContext) const override;
};