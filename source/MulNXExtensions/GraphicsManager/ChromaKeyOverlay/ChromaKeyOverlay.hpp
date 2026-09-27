#pragma once
#include <MulNXExtensions/GraphicsManager/GraphicsManager.hpp>
#include <MulNXExtensions/GraphicsManager/ShaderCompiler/ShaderCompiler.hpp>
#include <d3d11.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

class ChromaKeyOverlay final :public MulNX::Module<ChromaKeyOverlay> {
    MulNX::GraphicsManager* pGraphicsManager = nullptr;
    MulNX::ShaderCompiler* pShaderCompiler = nullptr;

    // 深度副本
    ComPtr<ID3D11Texture2D> pDepthCopyTex = nullptr;
    ComPtr<ID3D11ShaderResourceView> pDepthSRV = nullptr;
    UINT m_DepthWidth = 0;
    UINT m_DepthHeight = 0;
    DXGI_FORMAT m_DepthCopyFormat = DXGI_FORMAT_UNKNOWN;

    // 颜色副本（后备缓冲区拷贝）
    ComPtr<ID3D11Texture2D> pColorCopyTex = nullptr;
    ComPtr<ID3D11ShaderResourceView> pColorCopySRV = nullptr;
    DXGI_FORMAT m_ColorCopyFormat = DXGI_FORMAT_UNKNOWN;
    UINT m_ColorWidth = 0;
    UINT m_ColorHeight = 0;

    // 绿幕着色器资源
    ID3D11VertexShader* m_pGreenVS = nullptr;
    ID3D11PixelShader* m_pGreenPS = nullptr;
    ID3D11InputLayout* m_pGreenLayout = nullptr;
    ID3D11Buffer* m_pGreenCB = nullptr;
    ID3D11SamplerState* m_pPointSampler = nullptr;
    ID3D11BlendState* m_pBlendState = nullptr;   // 可选，用于半透明混合   

    void EnsureCopyResources();   // 创建/重建颜色和深度副本
    void CopyColorBuffer();       // 从后备缓冲区拷贝到颜色副本
    void RenderGreenScreen();

    std::atomic<bool>enabled{ false };
    std::atomic<float>m_GreenThreshold{ 0.5 };// 绿幕阈值

    bool Menu();
    bool Init()override;

    void CreateGreenScreenAssets();

    void ReleaseOld();
    std::atomic<bool> needReBuild = false;
    void BuildNew();

    void OnClearDepthStencilView(ID3D11DeviceContext* pCtx, ID3D11DepthStencilView* pDSV, UINT ClearFlags);
    void OnPresent();
};