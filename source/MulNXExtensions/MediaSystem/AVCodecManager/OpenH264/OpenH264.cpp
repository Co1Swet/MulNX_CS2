#include "OpenH264.hpp"
#include <MulNX/Base/UI/UI.hpp>

void OpenH264Encoder::SettingsMenu() {
    // 基础编码特性
    this->profileCombo.Render("Profile", this->profile);
    this->coderCombo.Render("熵编码", this->coder);
    this->loopfilterCombo.Render("环路滤波", this->loopfilter);
    // 码率控制
    this->rc_modeCombo.Render("码率控制模式", this->rc_mode);
    this->allow_skip_framesCombo.Render("是否允许跳帧", this->allow_skip_frames);
    // 切片 / 打包
    MulNX::UI::SliderInt("最大 NAL 大小", this->max_nal_size, 0, 4 * 1024 * 1024);
}

bool OpenH264Encoder::Init() {
    av::Codec codec = av::findEncodingCodec("libopenh264");
    if (codec.isNull()) {
        this->unavailableReason = "FFmpeg 中找不到编码器 libopenh264";
        return true;
    }
    if (!codec.canEncode()) {
        this->unavailableReason = "FFmpeg 编码器 libopenh264 不支持编码";
        return true;
    }

    this->codec = codec;
    return true;
}
bool OpenH264Encoder::IsAvailable()const {
    return !this->codec.isNull() && this->codec.canEncode();
}

av::Codec* OpenH264Encoder::GetAVCodec() {
    return &this->codec;
}

av::PixelFormat OpenH264Encoder::GetPreferredPixelFormat()const {
    return av::PixelFormat(AV_PIX_FMT_YUV420P);
}

av::Dictionary OpenH264Encoder::GetPrivateOpts()const {
    av::Dictionary opts{};

    auto addComboOpt = [&opts](const char* key, const std::atomic<std::shared_ptr<std::string>>& value) {
        auto selected = value.load(std::memory_order_acquire);
        if (selected && !selected->empty() && *selected != "默认") {
            opts[key].set(*selected);
        }
        };

    // 基础编码特性
    addComboOpt("profile", this->profile);
    addComboOpt("coder", this->coder);
    addComboOpt("loopfilter", this->loopfilter);

    // 码率控制
    addComboOpt("rc_mode", this->rc_mode);
    addComboOpt("allow_skip_frames", this->allow_skip_frames);

    // 切片 / 打包
    if (this->max_nal_size > 0) {
        opts["max_nal_size"].set(std::to_string(this->max_nal_size));
    }

    return opts;
}