#include "H264Mf.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <algorithm>

void H264Mf::SettingsMenu() {
    this->rateControlCombo.Render("码率控制", this->rate_control);
    this->scenarioCombo.Render("编码场景", this->scenario);
    MulNX::UI::SliderInt("编码质量 (-1=默认)", this->quality, -1, 100);
    MulNX::UI::Checkbox("强制硬件编码", this->hw_encoding);
}

bool H264Mf::Init() {
    this->codec = av::findEncodingCodec("h264_mf");
    if (this->codec.isNull()) {
        this->unavailableReason = "FFmpeg 中找不到编码器 h264_mf";
        return true;
    }
    if (!this->codec.canEncode()) {
        this->unavailableReason = "FFmpeg 编码器 h264_mf 不支持编码";
        return true;
    }

    try {
        av::VideoEncoderContext enc(this->codec);
        enc.setWidth(1280);
        enc.setHeight(720);
        enc.setPixelFormat(this->GetPreferredPixelFormat());
        enc.setTimeBase(av::Rational(1, 30));
        enc.setBitRate(2'000'000);
        enc.setGopSize(30);
        enc.setMaxBFrames(0);
        auto opts = this->PrepareOpen(&enc);
        enc.open(opts);
        this->ok.store(true, std::memory_order_release);
    }
    catch (const std::exception& e) {
        this->unavailableReason = e.what();
        this->LogWarning(std::format("Media Foundation H.264 不可用：{}", e.what()));
    }

    return true;
}

bool H264Mf::IsAvailable() const {
    return this->ok.load(std::memory_order_acquire);
}

av::Codec* H264Mf::GetAVCodec() {
    return &this->codec;
}

av::PixelFormat H264Mf::GetPreferredPixelFormat() const {
    return av::PixelFormat(AV_PIX_FMT_NV12);
}

av::Dictionary H264Mf::PrepareOpen(av::VideoEncoderContext* ctx) const {
    auto* raw = ctx->raw();

    raw->framerate = AVRational{ 30, 1 };
    // mfenc.c 默认 Baseline，且源码强制开 CABAC → 冲突 + 压缩率最低
    raw->profile = AV_PROFILE_H264_HIGH;

    av::Dictionary opts;
    auto selRc = this->rate_control.load(std::memory_order_acquire);
    const bool isQuality = selRc && *selRc == "quality";
    if (selRc && !selRc->empty()) opts["rate_control"].set(*selRc);

    auto selSc = this->scenario.load(std::memory_order_acquire);
    if (selSc && !selSc->empty()) opts["scenario"].set(*selSc);

    if (isQuality) {
        // CQ 走裸 QP（16–51），不乘 FF_QP2LAMBDA；
        // 且不要再设 opts["quality"]（1–100 的 AVEncCommonQuality，两套旋钮会打架）
        raw->global_quality = 20;
    }
    else {
        const int q = this->quality.load(std::memory_order_acquire);
        if (q >= 0) opts["quality"].set(std::to_string(std::clamp(q, 0, 100)));
    }

    opts["hw_encoding"].set(this->hw_encoding.load(std::memory_order_acquire) ? "1" : "0");
    return opts;
}