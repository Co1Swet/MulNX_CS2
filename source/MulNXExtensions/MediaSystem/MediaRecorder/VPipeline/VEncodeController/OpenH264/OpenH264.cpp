#include "OpenH264.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/MediaSystem/MediaRecorder/VPipeline/VEncodeController/VEncodeController.hpp>

void OpenH264Encoder::Menu() {
    // 基础编码特性
    this->profile.Render("Profile");
    this->coder.Render("熵编码");
    this->loopfilter.Render("环路滤波");

    // 码率控制
    this->rc_mode.Render("码率控制模式");
    this->allow_skip_frames.Render("是否允许跳帧");

    // 切片 / 打包
    if (ImGui::InputInt("最大 NAL 大小", &this->max_nal_size)) {
        if (this->max_nal_size < 0) this->max_nal_size = 0;
    }
}

bool OpenH264Encoder::Init() {
    av::Codec codec = av::findEncodingCodec("libopenh264");
    if (codec.isNull()) {
        MulNX::ErrorTerminate("找不到编码器 libopenh264");
    }
    if (!codec.canEncode()) {
        MulNX::ErrorTerminate("编码器 libopenh264 不支持编码");
    }

    this->FindModule<VEncodeController>("VEncodeController")->pEncoder = this;

    this->UIRegisterCallback("AV.VCodec", [this](auto&&...) {
        this->Menu();
        });

    this->codec = codec;
    return true;
}

av::Codec* OpenH264Encoder::GetAVCodec() {
    return &this->codec;
}

av::Dictionary OpenH264Encoder::GetPrivateOpts() {
    av::Dictionary opts{};

    auto addComboOpt = [&opts](const char* key, StringCombo& combo) {
        const char* sel = combo.GetSelected();
        if (sel && std::string(sel) != "默认") {
            opts[key].set(std::string(sel));
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