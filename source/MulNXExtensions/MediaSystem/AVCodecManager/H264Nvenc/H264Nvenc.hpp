#pragma once
#include <MulNXExtensions/MediaSystem/AVCodecManager/AVCodecManager.hpp>

class H264Nvenc final : public MediaModuleBase, public VEncoderMixin<H264Nvenc> {
    class MediaParamManager* pMediaParamManager = nullptr;
    MulReflInt(preset, 0);      // 1=p1 最快
    MulReflInt(tune, 0);        // 0=ull
    MulReflInt(qp, 20);         // constqp 质量
    MulReflInt(qmax, 25);       // 防色块核心
    MulReflInt(lookahead, 0);
    MulReflInt(surfaces, 4);
    av::Codec codec{};

    bool IsAvailable() const override;
    void SettingsMenu() override;
    bool Init() override;
    av::Codec* GetAVCodec() override;
    av::Dictionary GetPrivateOpts() const override;
    inline bool SupportsConstantQuality() const override { return true; }
};