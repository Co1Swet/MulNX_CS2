#pragma once
#include <Buildup/PlayerHub/CSViewPlayerModuleBase.hpp>
#include <MulNXUtils/WinExt/WinExt.hpp>
#include <set>

class TeamIDController final : public CSViewPlayerModuleBase {
    void HubWindow(MulNX::UINode* node);

    // Hook 句柄
    std::unique_ptr<MulNX::Hook> hkLoadFromFile = nullptr;
    std::unique_ptr<MulNX::Hook> hkWashColorParse = nullptr;
    std::unique_ptr<MulNX::Hook> hkWashColorClone = nullptr;
    std::unique_ptr<MulNX::Hook> hkPosTeamID_CmpForHide = nullptr;

    // 是否正在加载 hudreticle.xml
    bool inHudReticle = false;

    // 收集到的 WashColor 对象地址（this 指针）
    std::set<uintptr_t> tWashColors;
    std::set<uintptr_t> ctWashColors;

    MulNX::Hook::Then HandleForShowTeamID(CS2::C_CSPlayerPawn* pCSPlayerPawn);

    bool Init() override;
    // 设置 T 方头顶颜色，a=0 即可隐藏
    void SetTColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
    // 设置 CT 方头顶颜色
    void SetCTColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
    // 恢复默认颜色
    void ResetToDefault();
};