#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>

namespace MulNX {
    class IVEncoder {
    public:
        virtual bool IsAvailable() = 0;
        virtual const char* GetNotAvailableReason() { return "N/A"; }
        virtual av::Codec* GetAVCodec() = 0;
        virtual av::Dictionary GetPrivateOpts() = 0;
        virtual void SettingsMenu() = 0;
        virtual bool SupportsConstantQuality()const = 0;
    };
}

class AVCodecManager final :public MediaModuleBase {
    std::vector<MulNX::IVEncoder*> encoders{};
    bool Init()override;
public:
    void AddEncoder(MulNX::IVEncoder* pEncoder);
    MulNX::IVEncoder* GetCurrentVEncoder();
};

template<typename T>
class VEncoderMixin :public MulNX::IVEncoder {
    T* This() { return static_cast<T*>(this); }
protected:
    AVCodecManager* pAVCodecManager = nullptr;
    VEncoderMixin() {
        This()->preInits.push_back([this]() {
            this->pAVCodecManager = This()->FindModule<AVCodecManager>("AVCodecManager");
            this->pAVCodecManager->AddEncoder(this);
            return true;
            });
    }
};