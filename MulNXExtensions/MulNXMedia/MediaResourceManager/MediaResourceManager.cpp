#include "MediaResourceManager.hpp"
#include <vector>



bool MediaResourceManager::Init() {
    this->pGraphicsManager = this->Core->ModuleManager()->FindModule<MulNX::GraphicsManager>("GraphicsManager");

    av::init();
    av::set_logging_level(AV_LOG_WARNING);

    this->ISys().LogSucc("FFmpeg 与 AvCpp 初始化成功！");

    this->ISys()
        .SubscribeAsync("Media/Record/Start")
        .SubscribeAsync("Media/Record/Stop")
        ;

    this->ISys().SubscribeSync("Hook/BeforePresent", [this](MulNX::Message& msg) {this->HandleOnPresent();});

    this->dirVedios = this->ISys().PathManager()->PathGetForShared("Vedios");
    return true;
}

void MediaResourceManager::ProcessMsg(MulNX::Message& msg) {
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

void MediaResourceManager::Deinit() {
    this->StopRecording();
}

void MediaResourceManager::ReleaseStagingTexture() {
    if (this->pStagingTex) {
        this->pStagingTex->Release();
        this->pStagingTex = nullptr;
    }
    this->stagingWidth = 0;
    this->stagingHeight = 0;
    this->stagingFormat = DXGI_FORMAT_UNKNOWN;
}

void MediaResourceManager::HandleOnPresent() {
    this->Update();
    if (!this->isRecording) return;
    
    ID3D11Texture2D* backBuffer = nullptr;
    this->pGraphicsManager->pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
    if (!backBuffer) {
        return;
    }

    D3D11_TEXTURE2D_DESC desc;
    backBuffer->GetDesc(&desc);

    av::PixelFormat srcFormat = DXGIFormatToAvPixelFormat(desc.Format);
    if (srcFormat == AV_PIX_FMT_NONE) {
        this->ISys().LogError("当前后备缓冲区格式不受支持，无法录制");
        return;
    }

    if (!this->pStagingTex || this->stagingWidth != desc.Width || this->stagingHeight != desc.Height || this->stagingFormat != desc.Format) {
        this->ReleaseStagingTexture();

        D3D11_TEXTURE2D_DESC stagingDesc = {};
        stagingDesc.Width = desc.Width;
        stagingDesc.Height = desc.Height;
        stagingDesc.MipLevels = 1;
        stagingDesc.ArraySize = 1;
        stagingDesc.Format = desc.Format;
        stagingDesc.SampleDesc.Count = 1;
        stagingDesc.Usage = D3D11_USAGE_STAGING;
        stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        stagingDesc.BindFlags = 0;

        HRESULT hr = this->pGraphicsManager->pd3dDevice->CreateTexture2D(&stagingDesc, nullptr, &this->pStagingTex);
        if (FAILED(hr)) {
            this->ISys().LogError("创建 D3D11 staging 纹理失败，录制中断");
            return;
        }

        this->stagingWidth = desc.Width;
        this->stagingHeight = desc.Height;
        this->stagingFormat = desc.Format;
        this->srcPixelFormat = srcFormat;
    }

    // Copy back buffer into staging texture for CPU readback.
    if (desc.SampleDesc.Count > 1) {
        this->pGraphicsManager->pd3dContext->ResolveSubresource(this->pStagingTex, 0, backBuffer, 0, desc.Format);
    }
    else {
        this->pGraphicsManager->pd3dContext->CopyResource(this->pStagingTex, backBuffer);
    }
    backBuffer->Release();

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    HRESULT hr = this->pGraphicsManager->pd3dContext->Map(this->pStagingTex, 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(hr) || !mapped.pData) {
        this->ISys().LogError("Map D3D11 staging 纹理失败，录制帧跳过");
        return;
    }

    size_t rowBytes = static_cast<size_t>(this->stagingWidth) * 4;
    std::vector<uint8_t> rawData(rowBytes * this->stagingHeight);
    uint8_t* dstRow = rawData.data();
    for (UINT row = 0; row < static_cast<UINT>(this->stagingHeight); ++row) {
        const uint8_t* srcRow = reinterpret_cast<const uint8_t*>(mapped.pData) + static_cast<size_t>(mapped.RowPitch) * row;
        memcpy(dstRow, srcRow, rowBytes);
        dstRow += rowBytes;
    }
    this->pGraphicsManager->pd3dContext->Unmap(this->pStagingTex, 0);

    av::VideoFrame srcFrame = av::VideoFrame::wrap(rawData.data(), rawData.size(), srcFormat, this->stagingWidth, this->stagingHeight);
    av::VideoFrame dstFrame(AV_PIX_FMT_YUV420P, this->width, this->height);
    if (!this->rescaler.isValid() || this->rescaler.srcWidth() != this->width || this->rescaler.srcHeight() != this->height || this->rescaler.srcPixelFormat() != srcFormat) {
        this->rescaler = av::VideoRescaler(
            this->width, this->height, AV_PIX_FMT_YUV420P,
            this->stagingWidth, this->stagingHeight, srcFormat,
            av::SwsFlagFastBilinear
        );
    }

    try {
        this->rescaler.rescale(dstFrame, srcFrame);
        dstFrame.setPts(this->ptsCounter++, this->timeBase);

        av::Packet pkt = this->encoder.encode(dstFrame);
        if (pkt && pkt.size() > 0) {
            pkt.setStreamIndex(this->vstream.index());
            pkt.setTimeBase(this->vstream.timeBase());
            this->ofctx.writePacket(pkt);
        }
    }
    catch (const std::exception &e) {
        this->ISys().LogError(std::string("录制帧写入失败: ") + e.what());
    }
}

bool MediaResourceManager::StartRecording(const std::string& filename, int w, int h) {
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
    catch (const std::exception &e) {
        this->ISys().LogError(std::string("录制启动失败: ") + e.what());
        this->ofctx.close();
        return false;
    }
}

bool MediaResourceManager::StopRecording() {
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
    this->srcPixelFormat = AV_PIX_FMT_NONE;
    this->ReleaseStagingTexture();
    this->ofctx.close();
    return true;
}