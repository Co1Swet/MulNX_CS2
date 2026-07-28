#pragma once
#include <Intro/CSModuleBase.hpp>
#include <MulNX/Base/UI/UI.hpp>

namespace CS2 {
    class C_EnvSky :public C_BaseModelEntity {
    public:
        uint64_t* m_hSkyMaterial() { return Schema<uint64_t>(this, cs2_dumper::schemas::client_dll::C_EnvSky::m_hSkyMaterial); }
        uint64_t* m_hSkyMaterialLightingOnly() { return Schema<uint64_t>(this, cs2_dumper::schemas::client_dll::C_EnvSky::m_hSkyMaterialLightingOnly); }
        bool* m_bStartDisabled() { return Schema<bool>(this, cs2_dumper::schemas::client_dll::C_EnvSky::m_bStartDisabled); }
        uint32_t* m_vTintColor() { return Schema<uint32_t >(this, cs2_dumper::schemas::client_dll::C_EnvSky::m_vTintColor); }
        uint32_t* m_vTintColorLightingOnly() { return Schema<uint32_t >(this, cs2_dumper::schemas::client_dll::C_EnvSky::m_vTintColorLightingOnly); }
        float* m_flBrightnessScale() { return Schema<float>(this, cs2_dumper::schemas::client_dll::C_EnvSky::m_flBrightnessScale); }
        int32_t* m_nFogType() { return Schema<int32_t>(this, cs2_dumper::schemas::client_dll::C_EnvSky::m_nFogType); }
        float* m_flFogMinStart() { return Schema<float>(this, cs2_dumper::schemas::client_dll::C_EnvSky::m_flFogMinStart); }
        float* m_flFogMinEnd() { return Schema<float>(this, cs2_dumper::schemas::client_dll::C_EnvSky::m_flFogMinEnd); }
        float* m_flFogMaxStart() { return Schema<float>(this, cs2_dumper::schemas::client_dll::C_EnvSky::m_flFogMaxStart); }
        float* m_flFogMaxEnd() { return Schema<float>(this, cs2_dumper::schemas::client_dll::C_EnvSky::m_flFogMaxEnd); }
        bool* m_bEnabled() { return Schema<bool>(this, cs2_dumper::schemas::client_dll::C_EnvSky::m_bEnabled); }
    };
}

class SkyController final : public CSModuleBase {
    std::atomic<bool> enable = false;
    std::unique_ptr<MulNX::Hook> hkForceUpdateSkybox{};

    std::atomic<uint32_t> skyColor{ IM_COL32(0, 0, 0, 255) };  // 普通染色（RGBA）
    std::atomic<float> brightness{ 2.0f };

    void Menu();
    bool Init() override;
    void ProcessMsg(MulNX::Message& msg) override;
    MulNX::Hook::Then HandleForceUpdateSkybox(CS2::C_EnvSky* pEnvSky);
};