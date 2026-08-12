#pragma once
#include <Intro/CSModuleBase.hpp>
#include <Intro/HookConsole/HookConsole.hpp>

class GameSettingsManager final :public CSModuleBase {
    float FocusDistance{};
    float CrispRadius{};
    float BlurDistance{};

    float* r_dof_override_near_blurry{};// 近模糊
    float* r_dof_override_near_crisp{};// 近清晰
    float* r_dof_override_far_crisp{};// 远清晰
    float* r_dof_override_far_blurry{};// 远模糊

    void Window();
    bool SoundMenu();
    bool DofMenu();
    bool GameHudMenu();
    bool Init()override;
};