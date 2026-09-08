#include "VEncodeController.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <MediaParamManager/MediaParamManager.hpp>

#include "OpenH264/OpenH264.hpp"

void VEncodeController::Menu() {
    if (ImGui::CollapsingHeader("高级设置")) {
        this->encoders[0]->DrawSettingsUI();
    }
}

bool VEncodeController::Init() {
    this->pMediaParamManager = this->FindModule<MediaParamManager>("MediaParamManager");

    this->encoders.push_back(std::make_unique<OpenH264Encoder>());

    for (auto& encoder : this->encoders) {
        if (!encoder->Init()) {
            this->LogError(std::format("编码器 {} 初始化失败", encoder->GetAVCodec()->name()));
        }
        else {
            this->LogInfo(std::format("编码器 {} 初始化成功", encoder->GetAVCodec()->name()));
        }
    }

    this->SubscribeSync("MediaSync/Reset", [this](auto&&...) {
        this->Reset();
        });

    this->SubscribeSync("MediaSync/SetOn", [this](MulNX::Message& msg) {
        auto&& [info] = msg.Access<MulNX::AVStartInfo>();
        try {
            this->SetOn(info.pOutCtx);
        }
        catch (const std::exception& e) {
            this->LogError(std::format("视频编码器SetOn失败：{}", e.what()));
        }
        });

    this->UIRegisterCallback("UI.MediaSys", [this](auto&&...) {this->Menu();});

    this->SendTask("Check", "MediaState", [this]() {
        auto curSize = this->bufferVFrames.size_approx();
        this->bufferSize.store(curSize, std::memory_order_release);
        return true;
        });

    this->UIRegisterCallback("UI.MediaSys/Control", [this](auto&&...) {
        ImGui::Text(std::format("视频帧大致缓存(压力系数): {} 帧",
            this->bufferSize.load(std::memory_order_relaxed)).c_str());
            });

    return true;
}
void VEncodeController::SetEncoderParams(av::VideoEncoderContext* encoder) {
    auto& rp = *this->pMediaParamManager;
    this->width = rp.width > 0 ? rp.width :
        this->pGlobalVars->renderX.load(std::memory_order_acquire);
    this->height = rp.height > 0 ? rp.height :
        this->pGlobalVars->renderY.load(std::memory_order_acquire);

    encoder->setWidth(this->width);
    encoder->setHeight(this->height);
    encoder->setPixelFormat(this->dstPixFmt);

    auto* raw = encoder->raw();
    raw->color_range = AVCOL_RANGE_MPEG;
    raw->colorspace = AVCOL_SPC_BT709;
    raw->color_primaries = AVCOL_PRI_BT709;
    raw->color_trc = AVCOL_TRC_BT709;

    encoder->setTimeBase(this->timeBase);

    encoder->setMaxBFrames(rp.maxBFrames);
    encoder->setBitRate(rp.rc == RateControl::CQ ? 0 : rp.bitrate);

    if (rp.gopSize > 0) {
        encoder->setGopSize(rp.gopSize);
    }
    else if (rp.targetFPS > 0) {
        encoder->setGopSize(rp.targetFPS * 2);
    }
    else {
        encoder->setGopSize(120);
    }
    if (rp.rc == RateControl::CQ && rp.cq > 0) {
        encoder->setGlobalQuality(static_cast<int32_t>(rp.cq * FF_QP2LAMBDA));
    }

}

bool VEncodeController::OpenEncoder(av::FormatContext* oCtx, const av::Codec& codec) {
    this->encoder = av::VideoEncoderContext(codec);
    this->SetEncoderParams(&this->encoder);

    av::Dictionary opts = *this->encoders[0]->GetPrivateOpts();
    try {
        std::error_code ec;
        this->encoder.open(opts, ec);
        if (ec) {
            this->LogError(std::format("{} 打开失败: {}", this->encoder.codec().name(), ec.message()));
            return false;
        }
    }
    catch (const std::exception& e) {
        this->LogError(std::format("{} 打开异常: {}", this->encoder.codec().name(), e.what()));
        return false;
    }

    this->vstream = oCtx->addStream(this->encoder);
    this->vstream.setTimeBase(this->timeBase);
    this->vstream.setupEncodingParameters(this->encoder);
    return true;
}

void VEncodeController::SetOn(av::FormatContext* oCtx) {
    this->dstPixFmt = AV_PIX_FMT_YUV420P;

    auto codec = this->encoders[0]->GetAVCodec();
    if (!this->OpenEncoder(oCtx, *codec)) {
        this->LogError("编码器打开失败");
    }
    else {
        this->LogSucc(std::format("创建编码器上下文成功: {}", codec->name()));
    }

    this->LogInfo(std::format("目标分辨率: {}x{}", this->width.load(), this->height.load()));
    auto fps = this->pMediaParamManager->targetFPS.load(std::memory_order_acquire);
    this->LogInfo(std::format("目标帧率: {} fps", fps > 0 ? fps : 60));
    this->LogSucc(std::format("编码器已开启: {}", this->encoder.codec().name()));
}

void VEncodeController::CheckRescaler(int srcW, int srcH, av::PixelFormat srcFmt) {
    if (this->rescaler.isValid() &&
        this->rescaler.srcWidth() == srcW && this->rescaler.srcHeight() == srcH &&
        this->rescaler.srcPixelFormat() == srcFmt &&
        this->rescaler.dstWidth() == this->width && this->rescaler.dstHeight() == this->height &&
        this->rescaler.dstPixelFormat() == this->dstPixFmt) {
        return;
    }
    this->rescaler = av::VideoRescaler(this->width, this->height, this->dstPixFmt,
        srcW, srcH, srcFmt, av::SwsFlagBicubic);
}

std::optional<av::Packet> VEncodeController::Encode() {
    if (!this->encoder.isOpened()) return std::nullopt;

    av::VideoFrame srcFrame;
    if (!this->bufferVFrames.try_dequeue(srcFrame))return std::nullopt;

    try {
        auto srcFmtRaw = srcFrame.pixelFormat();
        int64_t inPts = srcFrame.pts().timestamp(this->timeBase);
        int srcW = srcFrame.width();
        int srcH = srcFrame.height();

        av::VideoFrame dst;

        // ── 软件帧（BGRA，来自 CPU 读回）──
        if (srcW == this->width && srcH == this->height && srcFmtRaw == this->dstPixFmt) {
            dst = std::move(srcFrame);
        }
        else {
            this->CheckRescaler(srcW, srcH, srcFmtRaw);
            dst = av::VideoFrame(this->dstPixFmt, this->width, this->height);
            this->rescaler.rescale(dst, srcFrame);
        }

        dst.setTimeBase(this->timeBase);
        dst.setPts(av::Timestamp(inPts, this->timeBase));
        dst.setStreamIndex(this->vstream.index());

        av::Packet pkt = this->encoder.encode(dst);
        if (!pkt || !(pkt.size() > 0)) return std::nullopt;
        pkt.setStreamIndex(this->vstream.index());
        pkt.setTimeBase(this->vstream.timeBase());
        return pkt;
    }
    catch (const std::exception& e) {
        this->LogError(std::string("编码失败: ") + e.what());
        return std::nullopt;
    }
}

std::optional<av::Packet> VEncodeController::TrySetOff() {
    return std::nullopt;
    if (!this->encoder.isOpened()) return std::nullopt;
    try {
        av::Packet pkt = this->encoder.encode();
        if (pkt && pkt.size() != 0) {
            pkt.setStreamIndex(this->vstream.index());
            pkt.setTimeBase(this->vstream.timeBase());
            return pkt;
        }
    }
    catch (const std::exception& e) {
        this->LogError(std::string("刷新失败: ") + e.what());
    }
    return std::nullopt;
}

void VEncodeController::Reset() {
    av::VideoFrame clear;
    while (this->bufferVFrames.try_dequeue(clear)) {

    }
}