#include "VEncodeHelper.hpp"

bool VEncodeHelper::Init() {

    return true;
}

void VEncodeHelper::SetOn(av::FormatContext* oCtx, int width, int height, AVRational timeBase) {    
    this->width = width;
    this->height = height;
    this->timeBase = timeBase;

    this->ptsCounter = 0;

    av::Codec vcodec = av::findEncodingCodec(AV_CODEC_ID_H264);
    if (!vcodec.canEncode()) {
        this->ISys().LogError("未找到可用的 H264 编码器");
        return;
    }

    this->encoder = av::VideoEncoderContext(vcodec);
    this->encoder.setWidth(this->width);
    this->encoder.setHeight(this->height);
    this->encoder.setPixelFormat(AV_PIX_FMT_YUV420P);
    this->encoder.setTimeBase(this->timeBase);
    this->encoder.setBitRate(12000000);
    this->encoder.open();

    this->vstream = oCtx->addStream(this->encoder);
    this->vstream.setTimeBase(this->timeBase);
    this->vstream.setupEncodingParameters(this->encoder);
}

void VEncodeHelper::CheckRescaler(int srcWidth, int srcHeight, av::PixelFormat srcFormat) {
    if (!this->rescaler.isValid() ||
        this->rescaler.srcWidth() != this->width ||
        this->rescaler.srcHeight() != this->height ||
        this->rescaler.srcPixelFormat() != srcFormat) {
        this->rescaler = av::VideoRescaler(
            this->width, this->height, AV_PIX_FMT_YUV420P,
            srcWidth, srcHeight, srcFormat,
            av::SwsFlagFastBilinear
        );
    }
}

std::optional<av::Packet> VEncodeHelper::Encode(av::VideoFrame srcFrame) {
    try {
        av::VideoFrame dstFrame(AV_PIX_FMT_YUV420P, this->width, this->height);

        this->rescaler.rescale(dstFrame, srcFrame);
        dstFrame.setTimeBase(this->timeBase);

        int64_t ptsValue = this->ptsCounter++;
        if (srcFrame.pts().isValid()) {
            ptsValue = srcFrame.pts().timestamp(this->timeBase);
        }
        dstFrame.setPts(av::Timestamp(ptsValue, this->timeBase));
        dstFrame.setStreamIndex(this->vstream.index());

        av::Packet pkt = this->encoder.encode(dstFrame);
        if (!pkt || !(pkt.size() > 0))return std::nullopt;

        pkt.setStreamIndex(this->vstream.index());
        pkt.setTimeBase(this->vstream.timeBase());

        return pkt;
    }
    catch (const std::exception& e) {
        this->ISys().LogError(std::string("视频帧写入失败: ") + e.what());
        return std::nullopt;
    }
}

std::optional<av::Packet> VEncodeHelper::TrySetOff() {
    // 刷新视频编码器
    av::Packet pkt = this->encoder.encode();
    if (pkt && pkt.size() != 0) {
        pkt.setStreamIndex(this->vstream.index());
        pkt.setTimeBase(this->vstream.timeBase());
        return pkt;
    }
    
    this->width = 0;
    this->height = 0;

    this->ptsCounter = 0;

    return std::nullopt;
}