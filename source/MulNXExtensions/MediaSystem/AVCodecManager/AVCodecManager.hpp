#pragma once
#include <MulNXExtensions/MediaSystem/MediaModuleBase.hpp>
#include <string_view>

namespace MulNX {
    class IVEncoder {
    public:
        virtual std::string_view GetName()const = 0;
        virtual bool IsAvailable()const = 0;
        virtual const char* GetNotAvailableReason()const = 0;

        virtual av::Codec* GetAVCodec() = 0;
        virtual bool SupportsConstantQuality()const = 0;
        virtual av::PixelFormat GetPreferredPixelFormat()const = 0;

        virtual av::Dictionary GetPrivateOpts()const = 0;
        
        virtual void SettingsMenu() = 0;
        
    };
}

class AVCodecManager final :public MediaModuleBase {
    std::vector<MulNX::IVEncoder*> encoders{};
    MulReflStr(currentEncoderName, "H264Nvenc");
    MulReflStr(encoderFallbackModuleNames, R"(["H264Nvenc","OpenH264Encoder"])");
    std::string selectionStatus;

    void Menu();

    void ResolveStartupSelection();
    MulNX::IVEncoder* FindEncoder(std::string_view name) const;
    bool Init()override;
public:
    void AddEncoder(MulNX::IVEncoder* pEncoder);
    MulNX::IVEncoder* GetCurrentVEncoder();
};

template<typename T>
class VEncoderMixin :public MulNX::IVEncoder {
    T* This() { return static_cast<T*>(this); }
    const T* This() const { return static_cast<const T*>(this); }
protected:
    AVCodecManager* pAVCodecManager = nullptr;
    std::string unavailableReason = "未提供不可用原因";

    VEncoderMixin() {
        This()->preInits.push_back([this]() {
            this->pAVCodecManager = This()->FindModule<AVCodecManager>("AVCodecManager");
            this->pAVCodecManager->AddEncoder(this);
            return true;
            });
    }
public:
    inline std::string_view GetName()const override {
        return This()->GetNameView();
    }
    inline const char* GetNotAvailableReason()const override {
        return this->unavailableReason.c_str();
    }
};