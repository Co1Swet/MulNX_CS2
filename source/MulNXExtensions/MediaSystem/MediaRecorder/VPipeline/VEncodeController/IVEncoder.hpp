#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

namespace MulNX {
    class IVEncoder {
    public:
        virtual ~IVEncoder() = default;
        virtual bool Init() = 0;
        virtual void DrawSettingsUI() = 0;
        virtual av::Codec* GetAVCodec() = 0;
        virtual const av::Dictionary* GetPrivateOpts() = 0;
    };
}