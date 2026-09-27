#pragma once
#include <MulNX/MulNX.hpp>
#include <MulNXExtensions/GraphicsManager/ShaderCompiler/ShaderCompiler.hpp>
#include <d3d11.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

namespace MulNX {
    class GraphicsManager final :public MulNX::Module<GraphicsManager> {
        bool Init()override;
    public:
        // D3D11 核心指针
        ID3D11Device* pd3dDevice = nullptr;
        IDXGISwapChain* pSwapChain = nullptr;
        ID3D11DeviceContext* pd3dContext = nullptr;

        // 视图指针
        ComPtr<ID3D11RenderTargetView> view = nullptr;
    };
}