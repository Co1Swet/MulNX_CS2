#include "H264Nvenc.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/MediaSystem/MediaRecorder/MediaParamManager/MediaParamManager.hpp>

void H264Nvenc::SettingsMenu() {
    MulNX::UI::SliderInt("NVENC 预设 (1=p1最快, 7=p7最好)", this->preset, 1, 7);
    MulNX::UI::SliderInt("QP (CQ 模式)", this->qp, 1, 51);
    MulNX::UI::SliderInt("QP 上限（防色块）", this->qmax, 1, 51);
    MulNX::UI::SliderInt("前瞻帧数", this->lookahead, 0, 32);
    MulNX::UI::SliderInt("内部缓冲帧", this->surfaces, 1, 16);
}

bool H264Nvenc::Init() {
    this->pMediaParamManager = this->FindModule<MediaParamManager>();
    av::Codec c = av::findEncodingCodec("h264_nvenc");
    if (c.isNull()) {
        this->unavailableReason = "FFmpeg 未编入 h264_nvenc";
        return true;
    }
    if (!c.canEncode()) {
        this->unavailableReason = "h264_nvenc 不支持编码";
        return true;
    }
    this->codec = c;
    return true;
}

bool H264Nvenc::IsAvailable() const { return !this->codec.isNull() && this->codec.canEncode(); }
av::Codec* H264Nvenc::GetAVCodec() { return &this->codec; }
av::PixelFormat H264Nvenc::GetPreferredPixelFormat()const {
    return av::PixelFormat(AV_PIX_FMT_NV12);
}

av::Dictionary H264Nvenc::GetPrivateOpts() const {
    av::Dictionary opts;
    const char* presets[] = { "p1","p2","p3","p4","p5","p6","p7" };
    opts["preset"].set(std::string(presets[std::clamp(this->preset.load() - 1, 0, 6)]));
    opts["tune"].set(std::string(this->tune.load() == 0 ? "ull" : "hq"));
    opts["zerolatency"].set("1");
    opts["delay"].set("0");
    opts["surfaces"].set(std::to_string(this->surfaces.load()));
    opts["rc-lookahead"].set(std::to_string(this->lookahead.load()));

    auto* rp = this->pMediaParamManager;
    if (rp->rc == RateControl::CQ) {
        opts["rc"].set("constqp");
        opts["qp"].set(std::to_string(std::clamp(this->qp.load(), 1, 51)));
    }
    else if (rp && rp->rc == RateControl::CBR) {
        opts["rc"].set("cbr");
    }
    else {
        opts["rc"].set("vbr");
    }
    if (this->qmax.load() > 0) opts["qmax"].set(std::to_string(std::clamp(this->qmax.load(), 1, 51)));
    return opts;
}