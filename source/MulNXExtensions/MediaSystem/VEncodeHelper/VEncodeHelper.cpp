#include "VEncodeHelper.hpp"
#include <MulNXExtensions/MediaSystem/MediaParamManager/MediaParamManager.hpp>

bool VEncodeHelper::Init() {
    this->pMediaParamManager = this->FindModule<MediaParamManager>("MediaParamManager");

    this->SubscribeSync("MediaSync/Reset", [this](auto&&...) {
        this->Reset();
        });

    return true;
}

bool VEncodeHelper::OpenEncoder(av::FormatContext* oCtx, const av::Codec& codec, int fps, int srcW, int srcH) {
    auto& rp = *this->pMediaParamManager;
    this->width = (rp.width > 0) ? rp.width : srcW;
    this->height = (rp.height > 0) ? rp.height : srcH;
    this->chosenEncoder = codec.name();

    this->encoder = av::VideoEncoderContext(codec);
    this->encoder.setWidth(this->width);
    this->encoder.setHeight(this->height);
    this->encoder.setPixelFormat(this->dstPixFmt);
    this->encoder.setTimeBase(this->timeBase);

    int gop = rp.gopSize > 0 ? rp.gopSize : (fps > 0 ? fps * 2 : 120);
    this->encoder.setGopSize(gop);
    this->encoder.setMaxBFrames(rp.maxBFrames);
    this->encoder.setBitRate(rp.rc == RateControl::CQ ? 0 : rp.bitrate);
    if (rp.rc == RateControl::CQ && rp.cq > 0)
        this->encoder.setGlobalQuality(static_cast<int32_t>(rp.cq * FF_QP2LAMBDA));

    if (auto* raw = this->encoder.raw()) {
        raw->color_range = AVCOL_RANGE_MPEG;
        raw->colorspace = AVCOL_SPC_BT709;
        raw->color_primaries = AVCOL_PRI_BT709;
        raw->color_trc = AVCOL_TRC_BT709;
    }

    av::Dictionary opts;

    try {
        std::error_code ec;
        this->encoder.open(opts, ec);
        if (ec) { this->LogWarning(std::format("{} 打开失败: {}", this->chosenEncoder, ec.message())); return false; }
    }
    catch (const std::exception& e) {
        this->LogWarning(std::format("{} 打开异常: {}", this->chosenEncoder, e.what())); return false;
    }

    this->vstream = oCtx->addStream(this->encoder);
    this->vstream.setTimeBase(this->timeBase);
    this->vstream.setupEncodingParameters(this->encoder);
    return true;
}

void VEncodeHelper::SetOn(av::FormatContext* oCtx, int srcW, int srcH, av::PixelFormat srcFmt,
    ID3D11Device* device, int fps) {
    auto& rp = *this->pMediaParamManager;
    this->ptsCounter = 0;
    this->dstPixFmt = AV_PIX_FMT_YUV420P;

    AVCodecID targetId = (rp.mode == EncodeMode::HEVC) ? AV_CODEC_ID_HEVC : AV_CODEC_ID_H264;

    // 纯软件
    av::Codec codec = av::findEncodingCodec("libopenh264");
    if (codec.isNull()) codec = av::findEncodingCodec(targetId);
    if (!codec.isNull() && codec.canEncode() && this->OpenEncoder(oCtx, codec, fps, srcW, srcH)) {
        this->LogSucc(std::format("软件编码: {} ({}x{})", this->chosenEncoder, this->width, this->height));
        return;
    }
    this->LogError("没有可用的 H264/HEVC 编码器");
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

std::optional<av::Packet> VEncodeHelper::Encode(av::VideoFrame srcFrame) {
    if (!this->encoder.isOpened()) return std::nullopt;
    try {
        auto srcFmtRaw = srcFrame.pixelFormat();
        int64_t inPts = srcFrame.pts().isValid() ? srcFrame.pts().timestamp(this->timeBase) : -1;
        int srcW = srcFrame.width(), srcH = srcFrame.height();

        av::VideoFrame dst;

        // ── 软件帧（BGRA，来自 CPU 读回）──
        bool same = (srcW == this->width && srcH == this->height && srcFmtRaw == this->dstPixFmt);
        if (same) { dst = std::move(srcFrame); }
        else {
            this->CheckRescaler(srcW, srcH, srcFmtRaw);
            dst = av::VideoFrame(this->dstPixFmt, this->width, this->height);
            this->rescaler.rescale(dst, srcFrame);
        }

        int64_t pts = this->ptsCounter;
        if (inPts >= 0) pts = inPts;
        if (pts < this->ptsCounter) pts = this->ptsCounter;
        this->ptsCounter = pts + 1;
        dst.setTimeBase(this->timeBase);
        dst.setPts(av::Timestamp(pts, this->timeBase));
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
    this->ptsCounter = 0;
    if (this->encoder.isValid()) {
        if (auto* raw = this->encoder.raw()) { if (raw->hw_device_ctx) av_buffer_unref(&raw->hw_device_ctx); }
    }
}