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
        // 这个指针从Present函数等绑定，不要管引用计数
        IDXGISwapChain* pSwapChain = nullptr;
        
        ID3D11Device* pd3dDevice = nullptr;
        ID3D11DeviceContext* pd3dContext = nullptr;

        // 视图指针
        ComPtr<ID3D11RenderTargetView> refBackBufferView = nullptr;
    };
}