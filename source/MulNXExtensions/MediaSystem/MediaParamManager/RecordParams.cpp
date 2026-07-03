#include "RecordParams.hpp"
extern "C" {
#include <libavcodec/codec.h>
#include <libavutil/hwcontext.h>
}

EncoderCaps DetectEncoderCaps() {
    EncoderCaps caps;
    const AVCodec* codec = nullptr;
    void* iter = nullptr;
    while ((codec = av_codec_iterate(&iter)) != nullptr) {
        if (!av_codec_is_encoder(codec) || codec->type != AVMEDIA_TYPE_VIDEO) continue;
        std::string name(codec->name ? codec->name : "");
        if (name == "h264_nvenc" || name == "hevc_nvenc" ||
            name == "h264_amf"   || name == "hevc_amf"   ||
            name == "h264_qsv"   || name == "hevc_qsv")
            caps.hwEncoders.push_back(name);
        else
            caps.swEncoders.push_back(name);
    }
    AVHWDeviceType t = AV_HWDEVICE_TYPE_NONE;
    while ((t = av_hwdevice_iterate_types(t)) != AV_HWDEVICE_TYPE_NONE) {
        if (t == AV_HWDEVICE_TYPE_D3D11VA) { caps.d3d11vaAvailable = true; break; }
    }
    return caps;
}
