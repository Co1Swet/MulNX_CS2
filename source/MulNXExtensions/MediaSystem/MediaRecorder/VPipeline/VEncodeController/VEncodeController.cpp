#include "VEncodeController.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <MediaParamManager/MediaParamManager.hpp>
#include <MediaRecorder.hpp>

void VEncodeController::Menu(MulNX::UICoordinator* uico) {
    // if (ImGui::CollapsingHeader("高级设置")) {
    //     this->pEncoder->SettingsMenu();
    // }
}

bool VEncodeController::Init() {
    this->pMediaParamManager = this->FindModule<MediaParamManager>("MediaParamManager");
    this->pMediaRecorder = this->FindModule<MediaRecorder>("MediaRecorder");
    this->pAVCodecManager = this->FindModule<AVCodecManager>("AVCodecManager");

    this->SubscribeSync("MediaSync/SetOn", [this](MulNX::Message& msg) {
        auto&& [info] = msg.Access<MulNX::AVStartInfo>();
        try {
            this->SetOn(info.pOutCtx);
        }
        catch (const std::exception& e) {
            this->LogError(std::format("视频编码器SetOn失败：{}", e.what()));
        }
        });

    this->UIRegisterCallback("UI.MediaSys", [this](auto uico, auto&&...) {this->Menu(uico);});

    this->SendTask("Check", "MediaState", [this]() {
        auto curSize = this->bufferVFrames.size_approx();
        this->bufferSize.store(curSize, std::memory_order_release);
        return true;
        });

    this->UIRegisterCallback("UI.MediaSys/Control", [this](auto&&...) {
        ImGui::TextUnformatted(std::format("视频帧大致缓存(压力系数): {} 帧",
            this->bufferSize.load(std::memory_order_relaxed)).c_str());
        });

    return true;
}

std::optional<av::VideoEncoderContext> VEncodeController::OpenEncoder(const MediaParamManager& rp)const {
    auto iEncoder = this->pAVCodecManager->GetCurrentVEncoder();
    auto codec = iEncoder->GetAVCodec();
    auto vEncoderContext = av::VideoEncoderContext(*codec);

    vEncoderContext.setWidth(this->width);
    vEncoderContext.setHeight(this->height);
    // 格式处理
    vEncoderContext.setPixelFormat(iEncoder->GetPreferredPixelFormat());
    // 色彩空间处理
    auto* raw = vEncoderContext.raw();
    raw->color_range = AVCOL_RANGE_MPEG;
    raw->colorspace = AVCOL_SPC_BT709;
    raw->color_primaries = AVCOL_PRI_BT709;
    raw->color_trc = AVCOL_TRC_BT709;

    vEncoderContext.setTimeBase(this->timeBase);
    vEncoderContext.setMaxBFrames(rp.maxBFrames);

    if (rp.gopSize > 0) {
        vEncoderContext.setGopSize(rp.gopSize);
    }
    else if (rp.targetFPS > 0) {
        vEncoderContext.setGopSize(rp.targetFPS * 2);
    }
    else {
        vEncoderContext.setGopSize(120);
    }

    const bool wantCq = (rp.rc == RateControl::CQ);
    const bool cqOk = iEncoder->SupportsConstantQuality();
    vEncoderContext.setBitRate((wantCq && cqOk) ? 0 : rp.bitrate.load());

    av::Dictionary opts = iEncoder->GetPrivateOpts();
    try {
        std::error_code ec;
        vEncoderContext.open(opts, ec);
        if (ec) {
            this->LogError(std::format("{} 打开失败: {}", codec->name(), ec.message()));
            return std::nullopt;
        }
    }
    catch (const std::exception& e) {
        this->LogError(std::format("{} 打开异常: {}", codec->name(), e.what()));
        return std::nullopt;
    }
    return vEncoderContext;
}

void VEncodeController::SetOn(av::FormatContext* oCtx) {
    this->rescaleFrame.reset();
    CtrlVideoFrame ctrlVFrame;
    while (this->bufferVFrames.try_dequeue(ctrlVFrame)) {

    }
    this->pMediaState->encodingOverload = false;

    auto& rp = *this->pMediaParamManager;
    this->width = rp.width > 0 ? rp.width.load(std::memory_order_acquire) :
        this->pGlobalVars->renderX.load(std::memory_order_acquire);
    this->height = rp.height > 0 ? rp.height.load(std::memory_order_acquire) :
        this->pGlobalVars->renderY.load(std::memory_order_acquire);

    auto o = this->OpenEncoder(rp);
    if (!o) {
        this->LogError("编码器打开失败");
        return;
    }

    this->vEncoderContext = std::move(o.value());
    this->dstPixFmt = this->vEncoderContext.pixelFormat();
    this->vstream = oCtx->addStream(this->vEncoderContext);
    this->vstream.setTimeBase(this->timeBase);
    this->vstream.setupEncodingParameters(this->vEncoderContext);

    this->LogInfo(std::format("目标分辨率: {}x{}", this->width.load(), this->height.load()));
    auto fps = this->pMediaParamManager->targetFPS.load(std::memory_order_acquire);
    this->LogInfo(std::format("目标帧率: {} fps", fps > 0 ? fps : 60));
    this->LogSucc(std::format("编码器已开启: {}", this->vEncoderContext.codec().name()));

    this->SendTask("VEncode", "VEncode", [this]() {
        return this->OnEncode();
        });
}

void VEncodeController::CheckRescaler(int srcW, int srcH, av::PixelFormat srcFmt) {
    if (this->rescaler.isValid() &&
        this->rescaler.srcWidth() == srcW && this->rescaler.srcHeight() == srcH &&
        this->rescaler.srcPixelFormat() == srcFmt &&
        this->rescaler.dstWidth() == this->width && this->rescaler.dstHeight() == this->height &&
        this->rescaler.dstPixelFormat() == this->dstPixFmt) {
        return;
    }
    const int flags = (srcW == this->width && srcH == this->height) ?
        SWS_BILINEAR : SWS_BICUBIC;
    this->rescaler = av::VideoRescaler(this->width, this->height, this->dstPixFmt,
        srcW, srcH, srcFmt, flags);
}

bool VEncodeController::ExitCode() {
    size_t count = 0;
    try {
        while (true) {
            av::Packet pkt = this->vEncoderContext.encode();
            if (!pkt || pkt.size() <= 0)break;
            ++count;
            pkt.setStreamIndex(this->vstream.index());
            pkt.setTimeBase(this->vstream.timeBase());
            this->pMediaRecorder->bufferPackets.enqueue(pkt);
        }
    }
    catch (const std::exception& e) {
        this->LogError(std::format("刷新失败: {}", e.what()));
    }
    this->LogSucc(std::format("成功刷新视频包：{}", count));
    this->pMediaState->encodingOverload = false;
    this->pMediaRecorder->bufferPackets.enqueue(MulNX::AVControl(MulNX::AVControl::Kind::Stop));
    return false;
}

bool VEncodeController::OnEncode() {
    CtrlVideoFrame ctrlVFrame;
    if (!this->bufferVFrames.try_dequeue(ctrlVFrame)) {
        this->pMediaState->encodingOverload = false;
        return true;
    }
    else if (this->bufferVFrames.size_approx() >= 200) {
        this->pMediaState->encodingOverload = true;
    }
    else {
        this->pMediaState->encodingOverload = false;
    }

    auto pAVControl = std::get_if<MulNX::AVControl>(&ctrlVFrame);
    if (pAVControl && pAVControl->kind == MulNX::AVControl::Kind::Stop) {
        return this->ExitCode();
    }

    auto pSrcFrame = std::get_if<av::VideoFrame>(&ctrlVFrame);
    if (!pSrcFrame)return this->ExitCode();
    auto& srcFrame = *pSrcFrame;

    try {
        auto srcFmtRaw = srcFrame.pixelFormat();
        int64_t inPts = srcFrame.pts().timestamp(this->timeBase);
        int srcW = srcFrame.width();
        int srcH = srcFrame.height();

        av::VideoFrame* dst = nullptr;

        if (srcW == this->width && srcH == this->height && srcFmtRaw == this->dstPixFmt) {
            dst = &srcFrame;
        }
        else {
            this->CheckRescaler(srcW, srcH, srcFmtRaw);

            if (!this->rescaleFrame ||
                this->rescaleFrame->width() != this->width ||
                this->rescaleFrame->height() != this->height ||
                this->rescaleFrame->pixelFormat() != this->dstPixFmt) {
                this->rescaleFrame.emplace(this->dstPixFmt, this->width, this->height);
            }
            this->rescaler.rescale(*this->rescaleFrame, srcFrame);
            dst = &*this->rescaleFrame;
        }

        dst->setTimeBase(this->timeBase);
        dst->setPts(av::Timestamp(inPts, this->timeBase));
        dst->setStreamIndex(this->vstream.index());

        av::Packet pkt = this->vEncoderContext.encode(*dst);
        if (!pkt || pkt.size() <= 0)return true;
        pkt.setStreamIndex(this->vstream.index());
        pkt.setTimeBase(this->vstream.timeBase());
        this->pMediaRecorder->bufferPackets.enqueue(pkt);
    }
    catch (const std::exception& e) {
        this->LogError(std::format("编码失败: {}", e.what()));
    }
    return true;
}