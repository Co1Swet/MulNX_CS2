#include "ShaderCompiler.hpp"
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

// ---------- 着色器源码 ----------
// 全屏三角形顶点着色器
static const char* g_GreenVS = R"(
struct VSOut { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };
VSOut main(uint id : SV_VertexID) {
    VSOut o;
    o.uv = float2((id << 1) & 2, id & 2);
    o.pos = float4(o.uv * float2(2, -2) + float2(-1, 1), 0.0f, 1.0f);
    return o;
}
)";

// 像素着色器：深度阈值抠像
static const char* g_GreenPS = R"(
Texture2D<float>  DepthTex   : register(t0);
Texture2D<float4> ColorTex   : register(t1);
SamplerState      PointSampler : register(s0);

cbuffer ParamCB : register(b0) {
    float g_Threshold;
    float3 pad;
};

float4 main(float4 pos : SV_POSITION) : SV_Target {
    float depth = DepthTex.Load(int3(pos.xy, 0));
    float4 color = ColorTex.Load(int3(pos.xy, 0));
    if (depth < g_Threshold) {
        return color;                   // 前景：原色
    } else {
        return float4(0.0, 1.0, 0.0, 1.0); // 背景：纯绿
    }
}
)";

// ---------- 辅助：编译着色器 ----------
static ID3DBlob* CompileShader(const char* src, const char* target, const char* entry) {
    ID3DBlob* blob = nullptr, * errorBlob = nullptr;
    HRESULT hr = D3DCompile(src, strlen(src), nullptr, nullptr, nullptr,
        entry, target, 0, 0, &blob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            errorBlob->Release();
        }
        return nullptr;
    }
    if (errorBlob) errorBlob->Release();
    return blob;
}

bool MulNX::ShaderCompiler::Init() {
    this->ISys().LogInfo(I18n("graph.shader.compiling"));
    this->vsBlob = CompileShader(g_GreenVS, "vs_4_0", "main");
    this->psBlob = CompileShader(g_GreenPS, "ps_4_0", "main");
    if (!vsBlob || !psBlob) MulNX::ErrorTerminate(I18n("graph.shader.compile_fail"));
    this->ISys().LogSucc(I18n("graph.shader.compiled"));
    return true;
}

void MulNX::ShaderCompiler::Release() {
    this->vsBlob->Release();
    this->psBlob->Release();
}