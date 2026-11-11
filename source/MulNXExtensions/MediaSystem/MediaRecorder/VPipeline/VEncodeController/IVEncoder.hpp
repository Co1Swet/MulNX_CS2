#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

namespace MulNX {
    class IVEncoder {
    public:
        virtual av::Codec* GetAVCodec() = 0;
        virtual av::Dictionary GetPrivateOpts() = 0;
        virtual void SettingsMenu() = 0;
        virtual bool SupportsConstantQuality()const = 0;
    };
}