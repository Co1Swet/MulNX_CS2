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

av::Dictionary H264Mf::PrepareOpen(av::VideoEncoderContext* vEncoderContext) const {
    vEncoderContext->raw()->framerate = AVRational({ 30,1 });

    av::Dictionary opts;
    auto addComboOpt = [&opts](const char* key, const std::atomic<std::shared_ptr<std::string>>& value) {
        auto selected = value.load(std::memory_order_acquire);
        if (selected && !selected->empty()) {
            opts[key].set(*selected);
        }
    };

    addComboOpt("rate_control", this->rate_control);
    addComboOpt("scenario", this->scenario);

    const int selectedQuality = this->quality.load(std::memory_order_acquire);
    if (selectedQuality >= 0) {
        opts["quality"].set(std::to_string(std::clamp(selectedQuality, 0, 100)));
    }
    opts["hw_encoding"].set(this->hw_encoding.load(std::memory_order_acquire) ? "1" : "0");
    return opts;
}
