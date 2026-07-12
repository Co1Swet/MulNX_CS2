#pragma once
#include <Intro/CSModuleBase.hpp>
#include <MulNX/Base/UI/UI.hpp>
#include <map>

class TeamIDColorController final : public CSModuleBase {
    class WashColor {
    public:
        char pad[0x10];
        uint32_t color; // 0x10
    };

    std::unique_ptr<MulNX::Hook> hkLoadFromFile = nullptr;
    std::unique_ptr<MulNX::Hook> hkWashColorParse = nullptr;
    std::unique_ptr<MulNX::Hook> hkWashColorClone = nullptr;

    // 是否正在加载 hudreticle.xml
    bool inHudReticle = false;

    // 收集到的 WashColor 对象地址（this 指针）
    std::set<WashColor*> tWashColors;
    std::set<WashColor*> ctWashColors;

    // 直接保存当前 T/CT 的 UI 反馈颜色，便于在面板中持续显示
    ImVec4 bufferTColor{ 0xEA / 255.0f, 0xBE / 255.0f, 0x54 / 255.0f, 1.0f }; // 默认颜色 #eabe54
    ImVec4 bufferCTColor{ 0x96 / 255.0f, 0xC8 / 255.0f, 0xFA / 255.0f, 1.0f }; // 默认颜色 rgb(150, 200, 250)

    // 设置 T 方头顶颜色，a=0 即可隐藏
    void SetTColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
    void ResetTColor() { this->SetTColor(0xEA, 0xBE, 0x54, 0xFF); } // 默认颜色 #eabe54
    // 设置 CT 方头顶颜色
    void SetCTColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
    void ResetCTColor() { this->SetCTColor(0x96, 0xC8, 0xFA, 0xFF); } // 默认颜色 rgb(150, 200, 250)

    void HubTeam(MulNX::Message* umsg);
    bool Init() override;
    void ProcessMsg(MulNX::Message& Msg) override;
};