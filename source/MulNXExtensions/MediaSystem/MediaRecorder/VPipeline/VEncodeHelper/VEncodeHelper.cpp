#include "VEncodeHelper.hpp"
#include <MediaParamManager/MediaParamManager.hpp>

bool VEncodeHelper::Init() {
    this->pMediaParamManager = this->FindModule<MediaParamManager>("MediaParamManager");

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

    return true;
}

bool VEncodeHelper::OpenEncoder(av::FormatContext* oCtx, const av::Codec& codec) {
    this->encoder = av::VideoEncoderContext(codec);
    this->chosenEncoder = codec.name();

    auto& rp = *this->pMediaParamManager;
    this->width = rp.width > 0 ? rp.width :
        this->pGlobalVars->renderX.load(std::memory_order_acquire);
    this->height = rp.height > 0 ? rp.height :
        this->pGlobalVars->renderY.load(std::memory_order_acquire);
    this->encoder.setWidth(this->width);
    this->encoder.setHeight(this->height);
    this->encoder.setPixelFormat(this->dstPixFmt);

    auto* raw = this->encoder.raw();
    raw->color_range = AVCOL_RANGE_MPEG;
    raw->colorspace = AVCOL_SPC_BT709;
    raw->color_primaries = AVCOL_PRI_BT709;
    raw->color_trc = AVCOL_TRC_BT709;

    this->encoder.setTimeBase(this->timeBase);

    this->encoder.setMaxBFrames(rp.maxBFrames);
    this->encoder.setBitRate(rp.rc == RateControl::CQ ? 0 : rp.bitrate);

    if (rp.gopSize > 0) {
        this->encoder.setGopSize(rp.gopSize);
    }
    else if (rp.targetFPS > 0) {
        this->encoder.setGopSize(rp.targetFPS * 2);
    }
    else {
        this->encoder.setGopSize(120);
    }
    if (rp.rc == RateControl::CQ && rp.cq > 0) {
        this->encoder.setGlobalQuality(static_cast<int32_t>(rp.cq * FF_QP2LAMBDA));
    }

    av::Dictionary opts;
    try {
        std::error_code ec;
        this->encoder.open(opts, ec);
        if (ec) {
            this->LogError(std::format("{} 打开失败: {}", this->chosenEncoder, ec.message()));
            return false;
        }
    }
    catch (const std::exception& e) {
        this->LogError(std::format("{} 打开异常: {}", this->chosenEncoder, e.what()));
        return false;
    }

    this->vstream = oCtx->addStream(this->encoder);
    this->vstream.setTimeBase(this->timeBase);
    this->vstream.setupEncodingParameters(this->encoder);
    return true;
}

void VEncodeHelper::SetOn(av::FormatContext* oCtx) {
    this->dstPixFmt = AV_PIX_FMT_YUV420P;

    // 纯软件
    av::Codec codec = av::findEncodingCodec("libopenh264");
    if (codec.isNull()){
        this->LogError("libopenh264 缺失");
        return;
    }
    if (!codec.canEncode()) {
        this->LogError("编码器无法编码");
        return;
    }
    if (!this->OpenEncoder(oCtx, codec)) {
        this->LogError("编码器打开失败");
    }

    this->LogInfo(std::format("目标分辨率: {}x{}",
        this->width.load(), this->height.load()));
    auto fps = this->pMediaParamManager->targetFPS.load(std::memory_order_acquire);
    this->LogInfo(std::format("目标帧率: {} fps",
        fps > 0 ? fps : 60));
    this->LogSucc(std::format("软件编码已开启: {} ({}x{})",
        this->chosenEncoder, this->width.load(), this->height.load()));
}

void VEncodeHelper::CheckRescaler(int srcW, int srcH, av::PixelFormat srcFmt) {
    if (this->rescaler.isValid() &&
        this->rescaler.srcWidth() == srcW && this->rescaler.srcHeight() == srcH &&
        this->rescaler.srcPixelFormat() == srcFmt &&
        this->rescaler.dstWidth() == this->width && this->rescaler.dstHeight() == this->height &&
        this->rescaler.dstPixelFormat() == this->dstPixFmt)
        return;
    this->rescaler = av::VideoRescaler(this->width, this->height, this->dstPixFmt,
        srcW, srcH, srcFmt, av::SwsFlagBicubic);
}

std::optional<av::Packet> VEncodeHelper::Encode() {
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

std::optional<av::Packet> VEncodeHelper::TrySetOff() {
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

void VEncodeHelper::Reset() {
    av::VideoFrame clear;
    while (this->bufferVFrames.try_dequeue(clear)) {
        
    }
}