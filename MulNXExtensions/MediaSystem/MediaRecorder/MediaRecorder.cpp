#include "MediaRecorder.hpp"
#include <MulNXExtensions/MediaSystem/FrameCapturer/FrameCapturer.hpp>

bool MediaRecorder::Init() {
    this->pGraphicsManager = this->Core->ModuleManager()->FindModule<MulNX::GraphicsManager>("GraphicsManager");
    this->pCapturer = this->Core->ModuleManager()->FindModule<FrameCapturer>("FrameCapturer");
    this->dirVedios = this->ISys().PathManager()->PathGetForShared("Vedios");

    this->ISys()
        .SubscribeAsync("Media/Record/Start")
        .SubscribeAsync("Media/Record/Stop")
        .SubscribeSync("Hook/BeforePresent", [this](MulNX::Message& msg) {this->HandleOnPresent();});
    
    return true;
}

void MediaRecorder::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Media/Record/Start"_hash: {
        auto outFile = this->dirVedios / "record.mp4";
        this->StartRecording(outFile.string(), 1920, 1080);
        break;
    }
    case "Media/Record/Stop"_hash: {
        this->StopRecording();
        break;
    }
    }
}

bool MediaRecorder::StartRecording(const std::string& filename, int w, int h) {
    if (this->isRecording) {
        this->ISys().LogWarning("已在录制中，StartRecording 被忽略");
        return false;
    }

    try {
        this->ofctx.openOutput(filename);

        av::Codec codec = av::findEncodingCodec(AV_CODEC_ID_H264);

        if (!codec.canEncode()) {
            this->ISys().LogError("未找到可用的 H264 编码器");
            return false;
        }

        this->encoder = av::VideoEncoderContext(codec);
        this->encoder.setWidth(w);
        this->encoder.setHeight(h);
        this->encoder.setPixelFormat(AV_PIX_FMT_YUV420P);
        this->encoder.setTimeBase(this->timeBase);
        this->encoder.setBitRate(4000000);
        this->encoder.open();

        this->vstream = this->ofctx.addStream(this->encoder);
        this->vstream.setTimeBase(this->timeBase);
        this->vstream.setupEncodingParameters(this->encoder);

        this->ofctx.writeHeader();

        this->width = w;
        this->height = h;
        this->isRecording = true;
        this->ptsCounter = 0;

        this->ISys().LogSucc("已开始录制: " + filename);
        return true;
    }
    catch (const std::exception& e) {
        this->ISys().LogError(std::string("录制启动失败: ") + e.what());
        this->ofctx.close();
        return false;
    }
}

void MediaRecorder::HandleOnPresent() {
    this->Update();
    if (!this->isRecording) return;

    static std::optional<std::chrono::steady_clock::time_point> lastCapture;
    auto now = std::chrono::steady_clock::now();
    constexpr std::chrono::duration<double> minInterval(1.0 / 60.0);

    if (lastCapture.has_value() && (now - *lastCapture < minInterval)) {
        return;
    }
    lastCapture = now;

    this->pCapturer->Captuer();
}

bool MediaRecorder::StopRecording() {
    if (!this->isRecording) {
        return false;
    }

    try {
        // flush encoder
        while (true) {
            av::Packet pkt = this->encoder.encode();
            if (!pkt || pkt.size() == 0) {
                break;
            }
            pkt.setStreamIndex(this->vstream.index());
            pkt.setTimeBase(this->vstream.timeBase());
            pkt.setDuration(1, this->vstream.timeBase());
            this->ofctx.writePacket(pkt);
        }
        this->ofctx.writeTrailer();
    }
    catch (const std::exception& e) {
        this->ISys().LogError(std::string("录制停止失败: ") + e.what());
    }

    this->isRecording = false;
    this->ptsCounter = 0;
    this->width = 0;
    this->height = 0;
    this->pCapturer->Reset();
    
    this->ofctx.close();
    return true;
}

void MediaRecorder::Encode() {
    auto opFrame = this->pCapturer->TryPop();
    if (!opFrame.has_value())return;
    auto srcFrame = opFrame.value();

    av::VideoFrame dstFrame(AV_PIX_FMT_YUV420P, this->width, this->height);
    if (!this->rescaler.isValid() ||
        this->rescaler.srcWidth() != this->width || this->rescaler.srcHeight() != this->height ||
        this->rescaler.srcPixelFormat() != this->pCapturer->srcPixelFormat) {
        this->rescaler = av::VideoRescaler(
            this->width, this->height, AV_PIX_FMT_YUV420P,
            this->pCapturer->stagingWidth, this->pCapturer->stagingHeight,
            this->pCapturer->srcPixelFormat, av::SwsFlagFastBilinear
        );
    }

    try {
        this->rescaler.rescale(dstFrame, srcFrame);
        dstFrame.setTimeBase(this->timeBase);
        dstFrame.setPts(av::Timestamp(this->ptsCounter++, this->timeBase));
        dstFrame.setStreamIndex(this->vstream.index());

        av::Packet pkt = this->encoder.encode(dstFrame);
        if (pkt && pkt.size() > 0) {
            pkt.setStreamIndex(this->vstream.index());
            pkt.setTimeBase(this->vstream.timeBase());
            pkt.setDuration(1, this->vstream.timeBase());
            this->ofctx.writePacket(pkt);
        }
    }
    catch (const std::exception& e) {
        this->ISys().LogError(std::string("录制帧写入失败: ") + e.what());
    }
}