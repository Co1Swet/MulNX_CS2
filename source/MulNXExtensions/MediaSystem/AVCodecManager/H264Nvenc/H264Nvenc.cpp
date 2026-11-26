#include "H264Nvenc.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/MediaSystem/MediaRecorder/MediaParamManager/MediaParamManager.hpp>

void H264Nvenc::SettingsMenu() {
    this->rcCombo.Render("码率控制", this->rc);
    MulNX::UI::SliderInt("NVENC 预设 (1=p1最快, 7=p7最好)", this->preset, 1, 7);
    MulNX::UI::SliderInt("QP (CQ 模式)", this->qp, 1, 51);
    MulNX::UI::SliderInt("QP 上限（防色块）", this->qmax, 1, 51);
    MulNX::UI::SliderInt("前瞻帧数", this->lookahead, 0, 32);
    MulNX::UI::SliderInt("内部缓冲帧", this->surfaces, 1, 16);
}

bool H264Nvenc::Init() {
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
    try {
        av::VideoEncoderContext enc = av::VideoEncoderContext(this->codec);

        enc.setWidth(1280);
        enc.setHeight(720);
        enc.setPixelFormat(av::PixelFormat(AV_PIX_FMT_NV12));
        enc.setTimeBase(av::Rational(1, 30));
        enc.setBitRate(2'000'000);
        enc.setGopSize(30);
        enc.setMaxBFrames(0);

        av::Dictionary opts = this->PrepareOpen(&enc);

        enc.open(opts);
        this->ok = true;
    }
    catch (const std::exception& e) {
        this->LogWarning(e.what());
        this->unavailableReason = e.what();
    }
    
    return true;
}

bool H264Nvenc::IsAvailable() const {
    return this->ok.load();
}
av::Codec* H264Nvenc::GetAVCodec() { return &this->codec; }
av::PixelFormat H264Nvenc::GetPreferredPixelFormat()const {
    return av::PixelFormat(AV_PIX_FMT_NV12);
}

av::Dictionary H264Nvenc::PrepareOpen(av::VideoEncoderContext* ctx) const {
    av::Dictionary opts;
    static const char* presets[] = { "p1","p2","p3","p4","p5","p6","p7" };
    opts["preset"].set(std::string(presets[std::clamp(this->preset.load() - 1, 0, 6)]));

    const int qmaxVal = this->qmax.load(std::memory_order_acquire);
    const int la = this->lookahead.load(std::memory_order_acquire);

    if (la > 0) {
        opts["rc-lookahead"].set(std::to_string(la));
        // 有前瞻就别锁零延迟：源码会按 buffer delay 裁 lookaheadDepth，
        // zerolatency=1 + delay=0 会直接打 "Lookahead not enabled" 并关掉它
    }
    else {
        opts["zerolatency"].set("1");
        opts["delay"].set("0");
    }
    opts["surfaces"].set(std::to_string(this->surfaces.load()));

    auto sel = this->rc.load(std::memory_order_acquire);
    const std::string rc = sel->empty() ? std::string("constqp") : *sel;

    if (rc == "constqp") {
        opts["rc"].set("constqp");
        opts["qp"].set(std::to_string(std::clamp(this->qp.load(), 1, 51)));
        ctx->setBitRate(0);          // CQ 必须 0，和 mf 正好相反
        // qmax 在这里别设：set_constqp() 会强制把 qmin/qmax 置 -1
    }
    else if (rc == "cbr") {
        opts["rc"].set("cbr");
        // 保留 controller 设的 rp.bitrate
        if (qmaxVal > 0) opts["qmax"].set(std::to_string(std::clamp(qmaxVal, 1, 51)));
    }
    else {
        opts["rc"].set("vbr");
        if (qmaxVal > 0) opts["qmax"].set(std::to_string(std::clamp(qmaxVal, 1, 51)));
    }
    return opts;
}