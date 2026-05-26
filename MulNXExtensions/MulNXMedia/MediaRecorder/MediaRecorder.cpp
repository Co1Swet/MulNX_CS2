#include "MediaRecorder.hpp"
#include <cstring>
#include <chrono>

// avcpp headers
#include <avcpp/av.h>
#include <avcpp/codec.h>
#include <avcpp/codeccontext.h>
#include <avcpp/formatcontext.h>
#include <avcpp/frame.h>
#include <avcpp/packet.h>
#include <avcpp/pixelformat.h>
#include <avcpp/rational.h>
#include <avcpp/timestamp.h>

Recorder::Recorder(const std::string& outputPath, int width, int height, int fps)
    : m_width(width), m_height(height), m_fps(fps)
    , m_frameCount(0), m_lastWidth(0), m_lastHeight(0) {
    av::init();  // 初始化 FFmpeg 库（注册组件、网络等）

    // 1. 创建输出格式上下文（MP4）
    m_formatCtx = std::make_unique<av::FormatContext>();
    m_formatCtx->setFormat(av::guessOutputFormat("mp4", outputPath, "video/mp4"));
    m_formatCtx->openOutput(outputPath);

    // 2. 配置视频编码器（H.264）
    av::VideoEncoderContext encCtx;
    encCtx.setCodec(av::findEncodingCodec("libx264"));
    encCtx.setWidth(width);
    encCtx.setHeight(height);
    encCtx.setPixelFormat(av::PixelFormat("bgra"));  // D3D11 默认 BGRA
    encCtx.setTimeBase(av::Rational(1, fps));
    encCtx.setBitRate(2'000'000);      // 2 Mbps
    encCtx.setGopSize(fps * 2);        // 每 2 秒一个关键帧
    encCtx.open();                     // 打开编码器（使用已设置的 codec）

    // 3. 添加视频流到容器
    m_stream = m_formatCtx->addStream(encCtx);
    m_stream.setTimeBase(av::Rational(1, fps));  // 设置流的时间基

    // 将编码器移动存储
    m_encoder = std::make_unique<av::VideoEncoderContext>(std::move(encCtx));

    // 4. 写入文件头
    m_formatCtx->writeHeader();
}

Recorder::~Recorder() {
    if (!m_formatCtx) return;

    // 1. 刷新编码器（发送 null 帧以取出缓冲的包）
    while (true) {
        auto pkt = m_encoder->encode();  // 无参 = 刷新
        if (!pkt.isComplete()) break;
        pkt.setStreamIndex(m_stream.index());
        pkt.setTimeBase(m_stream.timeBase());
        m_formatCtx->writePacket(pkt);
    }

    // 2. 写入文件尾
    m_formatCtx->writeTrailer();
}

void Recorder::captureAndEncode(IDXGISwapChain* pSwapChain) {
    if (!pSwapChain) return;

    // 获取 D3D11 设备和上下文
    Microsoft::WRL::ComPtr<ID3D11Device> pDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> pContext;
    HRESULT hr = pSwapChain->GetDevice(__uuidof(ID3D11Device), &pDevice);
    if (FAILED(hr)) return;
    pDevice->GetImmediateContext(&pContext);

    // 获取 back buffer
    Microsoft::WRL::ComPtr<ID3D11Texture2D> pBackBuffer;
    hr = pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &pBackBuffer);
    if (FAILED(hr)) return;

    D3D11_TEXTURE2D_DESC desc;
    pBackBuffer->GetDesc(&desc);

    // 创建或调整 staging texture（CPU 可读）
    if (!m_stagingTexture || m_lastWidth != desc.Width || m_lastHeight != desc.Height) {
        createStagingTexture(pDevice.Get(), desc);
        m_lastWidth = desc.Width;
        m_lastHeight = desc.Height;
    }

    // 复制 back buffer 到 staging
    pContext->CopyResource(m_stagingTexture.Get(), pBackBuffer.Get());

    // 映射 staging texture 到 CPU 内存
    D3D11_MAPPED_SUBRESOURCE mapped;
    hr = pContext->Map(m_stagingTexture.Get(), 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(hr)) return;

    // 创建 AVFrame 并填充像素数据
    auto frame = createFrameFromStaging(mapped);
    pContext->Unmap(m_stagingTexture.Get(), 0);

    if (!frame.isValid()) return;

    // 设置 PTS
    av::Timestamp pts(m_frameCount, av::Rational(1, m_fps));
    frame.setPts(pts);
    frame.setStreamIndex(m_stream.index());

    // 编码帧
    auto pkt = m_encoder->encode(frame);
    if (pkt.isComplete()) {
        pkt.setStreamIndex(m_stream.index());
        pkt.setTimeBase(m_stream.timeBase());
        m_formatCtx->writePacket(pkt);
    }

    ++m_frameCount;
}

void Recorder::createStagingTexture(ID3D11Device* pDevice, const D3D11_TEXTURE2D_DESC& desc) {
    D3D11_TEXTURE2D_DESC stagingDesc = desc;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stagingDesc.MiscFlags = 0;

    HRESULT hr = pDevice->CreateTexture2D(&stagingDesc, nullptr, &m_stagingTexture);
    if (FAILED(hr)) {
        m_stagingTexture.Reset();
        // 可在此抛出异常或记录错误
    }
}

av::VideoFrame Recorder::createFrameFromStaging(const D3D11_MAPPED_SUBRESOURCE& mapped) {
    // 创建带有内部缓冲区的 VideoFrame
    av::VideoFrame frame(av::PixelFormat("bgra"), m_lastWidth, m_lastHeight);
    if (!frame.isValid()) return av::VideoFrame::null();

    uint8_t* dst = frame.data(0);
    int dstStride = frame.raw()->linesize[0];  // FFmpeg 内部可能对齐
    const uint8_t* src = static_cast<const uint8_t*>(mapped.pData);
    size_t srcRowPitch = mapped.RowPitch;
    size_t copyWidth = m_lastWidth * 4;  // 每行字节数（BGRA）

    for (UINT y = 0; y < m_lastHeight; ++y) {
        memcpy(dst + y * dstStride, src + y * srcRowPitch, copyWidth);
    }

    return frame;
}