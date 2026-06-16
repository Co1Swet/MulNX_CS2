#pragma once
#include <MulNX/MulNX.hpp>
#include <d3d11.h>

namespace MulNX {
    class ShaderCompiler final :public MulNX::Module<ShaderCompiler> {
    public:
        ID3DBlob* vsBlob = nullptr;
        ID3DBlob* psBlob = nullptr;
        bool Init()override;
        void Release();
    };
}