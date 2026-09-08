#include "OpenH264.hpp"
#include <MulNX/Base/UI/UI.hpp>

bool OpenH264Encoder::Init() {
    av::Codec codec = av::findEncodingCodec("libopenh264");
    if (codec.isNull()) {
        MulNX::ErrorTerminate("找不到编码器 libopenh264");
    }
    if (!codec.canEncode()) {
        MulNX::ErrorTerminate("编码器 libopenh264 不支持编码");
    }

    this->opts = {
        {"rc_mode", "off"},
        {"coder", "0"},               // CAVLC
        {"profile", "high"},
        {"loopfilter", "1"},
        {"allow_skip_frames", "0"}
    };
    
    this->codec = codec;
    return true;
}

void OpenH264Encoder::DrawSettingsUI() {

}

av::Codec* OpenH264Encoder::GetAVCodec() {
    return &this->codec;
}

const av::Dictionary* OpenH264Encoder::GetPrivateOpts() {
    return &this->opts;
}