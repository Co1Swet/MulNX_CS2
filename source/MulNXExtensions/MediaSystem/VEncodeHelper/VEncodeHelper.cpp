#include "VEncodeHelper.hpp"
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_d3d11va.h>
#include <libavutil/pixfmt.h>

// 恢复正常顺序
static std::vector<std::string> HwList() { return { "h264_nvenc", "h264_amf", "h264_qsv" }; }
static std::vector<std::string> SwList() { return { "libx264", "libopenh264", "h264" }; }

static av::Codec FindFirst(const std::vector<std::string>& names) {
    for (const auto& n : names) {
        av::Codec c = av::findEncodingCodec(n);
        if (!c.isNull() && c.canEncode()) return c;
    }
    return {};
}

bool VEncodeHelper::Init() { return true; }

void VEncodeHelper::ApplyOpts(av::Dictionary& opts, const RecordParams& rp,
                               const std::string& encName) {
    if (rp.preset != "auto" && !rp.preset.empty()) {
        if (encName.find("nvenc") != std::string::npos) opts.set("preset", rp.preset);
        else if (encName.find("amf") != std::string::npos) opts.set("quality", rp.preset);
        else opts.set("preset", rp.preset);
    }
    if (encName.find("nvenc") != std::string::npos && rp.preset == "auto")
        opts.set("preset", "p4");

    if (encName.find("nvenc") != std::string::npos || encName.find("qsv") != std::string::npos) {
        switch (rp.rc) {
        case RateControl::CBR: opts.set("rc", "cbr"); break;
        case RateControl::VBR: opts.set("rc", "vbr"); break;
        case RateControl::CQ:  opts.set("rc", "vbr"); if (rp.cq > 0) opts.set("cq", std::to_string(rp.cq)); break;
        }
    }
    if (rp.rc == RateControl::CQ && encName.find("amf") != std::string::npos)
        opts.set("rc", "cqp");

    if (!rp.profile.empty() && rp.profile != "auto")
        opts.set("profile", rp.profile);

    if (encName.find("libx264") != std::string::npos) {
        opts.set("crf", std::to_string(rp.rc == RateControl::CQ ? rp.cq : 23));
        if (rp.preset != "auto" && !rp.preset.empty()) opts.set("preset", rp.preset);
    }
}

bool VEncodeHelper::SetupHwContext(ID3D11Device* device, int w, int h) {
    if (!device) return false;

    // 1. D3D11VA 设备上下文（包装捕获设备）
    this->hwD3D11DeviceRef = av_hwdevice_ctx_alloc(AV_HWDEVICE_TYPE_D3D11VA);
    if (!this->hwD3D11DeviceRef) return false;
    auto* devCtx = reinterpret_cast<AVHWDeviceContext*>(this->hwD3D11DeviceRef->data);
    auto* d3d = reinterpret_cast<AVD3D11VADeviceContext*>(devCtx->hwctx);
    device->AddRef();
    d3d->device = device;
    device->GetImmediateContext(&d3d->device_context);
    if (av_hwdevice_ctx_init(this->hwD3D11DeviceRef) < 0) { av_buffer_unref(&this->hwD3D11DeviceRef); return false; }

    // 2. D3D11VA 帧池
    this->hwD3D11FramesRef = av_hwframe_ctx_alloc(this->hwD3D11DeviceRef);
    if (!this->hwD3D11FramesRef) { av_buffer_unref(&this->hwD3D11DeviceRef); return false; }
    auto* fc = reinterpret_cast<AVHWFramesContext*>(this->hwD3D11FramesRef->data);
    fc->format = AV_PIX_FMT_D3D11;
    fc->sw_format = AV_PIX_FMT_NV12;
    fc->width = w; fc->height = h;
    fc->initial_pool_size = 6;
    auto* d3dFc = reinterpret_cast<AVD3D11VAFramesContext*>(fc->hwctx);
    d3dFc->BindFlags = D3D11_BIND_SHADER_RESOURCE;
    d3dFc->MiscFlags = 0;
    if (av_hwframe_ctx_init(this->hwD3D11FramesRef) < 0) {
        av_buffer_unref(&this->hwD3D11FramesRef); av_buffer_unref(&this->hwD3D11DeviceRef); return false;
    }

    // 3. 根据编码器决定目标 hw 格式
    const auto& enc = this->chosenEncoder;
    if (enc.find("amf") != std::string::npos) {
        this->hwInputPixFmt = AV_PIX_FMT_D3D11;
    } else if (enc.find("nvenc") != std::string::npos) {
        this->hwInputPixFmt = AV_PIX_FMT_CUDA;
        int ret = av_hwdevice_ctx_create_derived(&this->hwTargetDeviceRef, AV_HWDEVICE_TYPE_CUDA, this->hwD3D11DeviceRef, 0);
        if (ret < 0) {
            this->LogWarning(std::format("CUDA 上下文衍生失败({}), 尝试使用 D3D11VA 直传模式", ret));
            this->hwInputPixFmt = AV_PIX_FMT_D3D11;
            this->hwTargetDeviceRef = nullptr;
            this->hwTargetFramesRef = nullptr;
            return true;
        }
        this->hwTargetFramesRef = av_hwframe_ctx_alloc(this->hwTargetDeviceRef);
        if (!this->hwTargetFramesRef) { av_buffer_unref(&this->hwTargetDeviceRef); this->hwInputPixFmt = AV_PIX_FMT_NV12; return false; }
        auto* cfc = reinterpret_cast<AVHWFramesContext*>(this->hwTargetFramesRef->data);
        cfc->format = AV_PIX_FMT_CUDA; cfc->sw_format = AV_PIX_FMT_NV12;
        cfc->width = w; cfc->height = h; cfc->initial_pool_size = 6;
        if ((ret = av_hwframe_ctx_init(this->hwTargetFramesRef)) < 0) {
            this->LogWarning(std::format("CUDA FrameCtx失败({})", ret));
            av_buffer_unref(&this->hwTargetFramesRef); av_buffer_unref(&this->hwTargetDeviceRef);
            this->hwInputPixFmt = AV_PIX_FMT_NV12; return false;
        }
    } else if (enc.find("qsv") != std::string::npos) {
        this->hwInputPixFmt = AV_PIX_FMT_QSV;
        if (av_hwdevice_ctx_create_derived(&this->hwTargetDeviceRef, AV_HWDEVICE_TYPE_QSV, this->hwD3D11DeviceRef, 0) < 0) {
            this->hwInputPixFmt = AV_PIX_FMT_NV12; return false;
        }
        this->hwTargetFramesRef = av_hwframe_ctx_alloc(this->hwTargetDeviceRef);
        if (!this->hwTargetFramesRef) { av_buffer_unref(&this->hwTargetDeviceRef); this->hwInputPixFmt = AV_PIX_FMT_NV12; return false; }
        auto* qfc = reinterpret_cast<AVHWFramesContext*>(this->hwTargetFramesRef->data);
        qfc->format = AV_PIX_FMT_QSV; qfc->sw_format = AV_PIX_FMT_NV12;
        qfc->width = w; qfc->height = h; qfc->initial_pool_size = 6;
        if (av_hwframe_ctx_init(this->hwTargetFramesRef) < 0) {
            av_buffer_unref(&this->hwTargetFramesRef); av_buffer_unref(&this->hwTargetDeviceRef);
            this->hwInputPixFmt = AV_PIX_FMT_NV12; return false;
        }
    }
    return true;
}

bool VEncodeHelper::OpenEncoder(av::FormatContext* oCtx, const RecordParams& rp,
                                const av::Codec& codec, int fps, int srcW, int srcH) {
    this->width  = (rp.width  > 0) ? rp.width  : srcW;
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
        raw->color_range    = AVCOL_RANGE_MPEG; // 采用 Limited Range (16-235) 匹配硬件默认转换行为，解决发灰问题
        raw->colorspace     = AVCOL_SPC_BT709;
        raw->color_primaries = AVCOL_PRI_BT709;
        raw->color_trc      = AVCOL_TRC_BT709;
        // 硬件编码：挂对应设备上下文
        if (this->hwAccel) {
            AVBufferRef* devRef = this->hwTargetDeviceRef ? this->hwTargetDeviceRef : this->hwD3D11DeviceRef;
            raw->hw_device_ctx = av_buffer_ref(devRef);
        }
    }

    av::Dictionary opts;
    this->ApplyOpts(opts, rp, this->chosenEncoder);

    try {
        std::error_code ec;
        this->encoder.open(opts, ec);
        if (ec) { this->LogWarning(std::format("{} 打开失败: {}", this->chosenEncoder, ec.message())); return false; }
    } catch (const std::exception& e) {
        this->LogWarning(std::format("{} 打开异常: {}", this->chosenEncoder, e.what())); return false;
    }

    this->vstream = oCtx->addStream(this->encoder);
    this->vstream.setTimeBase(this->timeBase);
    this->vstream.setupEncodingParameters(this->encoder);
    return true;
}

void VEncodeHelper::SetOn(av::FormatContext* oCtx, const RecordParams& rp,
                          int srcW, int srcH, av::PixelFormat srcFmt,
                          ID3D11Device* device, int fps) {
    this->ptsCounter = 0;
    this->hwAccel = false;
    this->dstPixFmt = AV_PIX_FMT_NV12;
    this->hwInputPixFmt = AV_PIX_FMT_NONE;

    AVCodecID targetId = (rp.mode == EncodeMode::HEVC) ? AV_CODEC_ID_HEVC : AV_CODEC_ID_H264;

    // 硬件编码器：逐个尝试（零拷贝 → 软件帧 → 下一个）
    for (const auto& hwName : HwList()) {
        av::Codec codec = av::findEncodingCodec(hwName);
        if (codec.isNull() || !codec.canEncode()) continue;
        this->chosenEncoder = codec.name();

        // 尝试零拷贝
        if (this->SetupHwContext(device, (rp.width > 0 ? rp.width : srcW), (rp.height > 0 ? rp.height : srcH))) {
            this->hwAccel = true;
            this->dstPixFmt = this->hwInputPixFmt;
            if (this->OpenEncoder(oCtx, rp, codec, fps, srcW, srcH)) {
                this->LogSucc(std::format("硬件编码(零拷贝): {} ({}x{})", this->chosenEncoder, this->width, this->height));
                return;
            }
            if (this->encoder.isValid()) {
                if (auto* raw = this->encoder.raw()) { if (raw->hw_device_ctx) av_buffer_unref(&raw->hw_device_ctx); }
            }
            if (this->hwTargetFramesRef) av_buffer_unref(&this->hwTargetFramesRef);
            if (this->hwTargetDeviceRef) av_buffer_unref(&this->hwTargetDeviceRef);
            if (this->hwD3D11FramesRef) av_buffer_unref(&this->hwD3D11FramesRef);
            if (this->hwD3D11DeviceRef) av_buffer_unref(&this->hwD3D11DeviceRef);
        }
        // 零拷贝失败：同编码器 + NV12 软件帧
        this->hwAccel = false;
        this->dstPixFmt = AV_PIX_FMT_NV12;
        this->chosenEncoder = codec.name();
        if (this->OpenEncoder(oCtx, rp, codec, fps, srcW, srcH)) {
            this->LogSucc(std::format("硬件编码(软件帧): {} ({}x{})", this->chosenEncoder, this->width, this->height));
            return;
        }
    }

    // 纯软件回退
    av::Codec codec = FindFirst(SwList());
    if (codec.isNull()) codec = av::findEncodingCodec(targetId);
    if (!codec.isNull() && codec.canEncode() && this->OpenEncoder(oCtx, rp, codec, fps, srcW, srcH)) {
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

std::optional<av::VideoFrame> VEncodeHelper::AllocHwFrame() {
    if (!this->hwAccel || !this->hwD3D11FramesRef) return std::nullopt;
    av::VideoFrame hwf;
    int ret = av_hwframe_get_buffer(this->hwD3D11FramesRef, hwf.raw(), 0);
    if (ret < 0 || !hwf.raw()->data[0]) return std::nullopt;
    hwf.raw()->width = this->width;
    hwf.raw()->height = this->height;
    return hwf;
}

std::optional<av::Packet> VEncodeHelper::Encode(av::VideoFrame srcFrame) {
    if (!this->encoder.isOpened()) return std::nullopt;
    try {
        auto srcFmtRaw = srcFrame.pixelFormat();
        int64_t inPts = srcFrame.pts().isValid() ? srcFrame.pts().timestamp(this->timeBase) : -1;
        int srcW = srcFrame.width(), srcH = srcFrame.height();

        av::VideoFrame dst;

        if (srcFmtRaw == AV_PIX_FMT_D3D11) {
            // ── 硬件帧（来自 VideoCapturer::ReadbackHw）──
            if (this->hwInputPixFmt == AV_PIX_FMT_D3D11) {
                // AMF：直接送
                dst = std::move(srcFrame);
                if (dst.raw()->hw_frames_ctx == nullptr)
                    dst.raw()->hw_frames_ctx = av_buffer_ref(this->hwD3D11FramesRef);
            } else if (this->hwInputPixFmt == AV_PIX_FMT_CUDA || this->hwInputPixFmt == AV_PIX_FMT_QSV) {
                // NVENC/QSV：从 D3D11VA 转移到 CUDA/QSV
                dst = av::VideoFrame();
                dst.raw()->format = this->hwInputPixFmt;
                dst.raw()->width = this->width;
                dst.raw()->height = this->height;
                dst.raw()->hw_frames_ctx = av_buffer_ref(this->hwTargetFramesRef);
                int ret = av_hwframe_get_buffer(this->hwTargetFramesRef, dst.raw(), 0);
                if (ret < 0) { this->LogWarning("目标 hwframe 分配失败"); return std::nullopt; }
                ret = av_hwframe_transfer_data(dst.raw(), srcFrame.raw(), 0);
                if (ret < 0) { this->LogWarning(std::format("D3D11→{} 转移失败: {}", (int)this->hwInputPixFmt, ret)); return std::nullopt; }
            } else {
                return std::nullopt;
            }
        } else {
            // ── 软件帧（BGRA，来自 CPU 读回）──
            bool same = (srcW == this->width && srcH == this->height && srcFmtRaw == this->dstPixFmt);
            if (same) { dst = std::move(srcFrame); }
            else {
                this->CheckRescaler(srcW, srcH, srcFmtRaw);
                dst = av::VideoFrame(this->dstPixFmt, this->width, this->height);
                this->rescaler.rescale(dst, srcFrame);
            }
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
    } catch (const std::exception& e) {
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
    } catch (const std::exception& e) {
        this->LogError(std::string("刷新失败: ") + e.what());
    }
    return std::nullopt;
}

void VEncodeHelper::Reset() {
    this->ptsCounter = 0;
    this->hwAccel = false;
    this->hwInputPixFmt = AV_PIX_FMT_NONE;
    if (this->encoder.isValid()) {
        if (auto* raw = this->encoder.raw()) { if (raw->hw_device_ctx) av_buffer_unref(&raw->hw_device_ctx); }
    }
    if (this->hwTargetFramesRef) av_buffer_unref(&this->hwTargetFramesRef);
    if (this->hwTargetDeviceRef) av_buffer_unref(&this->hwTargetDeviceRef);
    if (this->hwD3D11FramesRef) av_buffer_unref(&this->hwD3D11FramesRef);
    if (this->hwD3D11DeviceRef) av_buffer_unref(&this->hwD3D11DeviceRef);
}
