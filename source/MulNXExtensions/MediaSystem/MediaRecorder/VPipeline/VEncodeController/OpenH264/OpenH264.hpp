#pragma once
#include "../IVEncoder.hpp"

class OpenH264Encoder final :public MulNX::IVEncoder {
    av::Codec codec{};
    av::Dictionary opts{};

    bool Init()override;
    void DrawSettingsUI()override;
    av::Codec* GetAVCodec()override;
    const av::Dictionary* GetPrivateOpts()override;
};