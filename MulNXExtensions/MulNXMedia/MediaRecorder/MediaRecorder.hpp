#pragma once

#include <MulNXExtensions/MulNXMedia/D3D11AV.hpp>

/**
 * @brief 视频录制器，在 D3D11 Present 循环中捕获帧并编码为 H.264/MP4
 */
class Recorder {
public:
    /**
     * @param outputPath 输出文件路径（例如 "output.mp4"）
     * @param width      视频宽度（应与 back buffer 宽度匹配）
     * @param height     视频高度
     * @param fps        帧率（用于 PTS 计算）
     */
    Recorder(const std::string& outputPath, int width, int height, int fps = 60);
    ~Recorder();

    // 禁止拷贝和移动（资源管理器）
    Recorder(const Recorder&) = delete;
    Recorder& operator=(const Recorder&) = delete;
    Recorder(Recorder&&) = delete;
    Recorder& operator=(Recorder&&) = delete;

    /**
     * @brief 捕获当前 swap chain 的 back buffer 并编码
     * @param pSwapChain D3D11 交换链指针
     * @note 应在 Present 调用之前或之后调用（推荐之后）
     */
    void captureAndEncode(IDXGISwapChain* pSwapChain);

private:
    void createStagingTexture(ID3D11Device* pDevice, const D3D11_TEXTURE2D_DESC& desc);
    av::VideoFrame createFrameFromStaging(const D3D11_MAPPED_SUBRESOURCE& mapped);

private:
    int m_width, m_height, m_fps;
    int64_t m_frameCount;
    UINT m_lastWidth, m_lastHeight;

    std::unique_ptr<av::FormatContext> m_formatCtx;
    std::unique_ptr<av::VideoEncoderContext> m_encoder;
    av::Stream m_stream;  // 由 FormatContext 管理，但需要存储索引等信息

    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_stagingTexture;
};