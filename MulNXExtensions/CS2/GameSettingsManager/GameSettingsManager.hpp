#pragma once

#include <MulNXExtensions/CS2/CSModuleBase.hpp>

class ScreenSettings {
public:
    int* spec_show_xray = nullptr;
    bool operator==(const ScreenSettings&)const = default;
};

class SoundSettings {
public:
    bool* snd_mute_losefocus = nullptr;//游戏窗口失去焦点时静音

    float* snd_menumusic_volume = nullptr; //主菜单音量
    float* snd_roundstart_volume = nullptr; //回合开始音量
    float* snd_roundaction_volume = nullptr; //回合开始行动音量
    float* snd_roundend_volume = nullptr; //回合结束音量
    float* snd_mvp_volume = nullptr; //MVP音量
    float* snd_mapobjective_volume = nullptr; //炸弹/人质音量
    float* snd_tensecondwarning_volume = nullptr; //十秒警告音量(0.04就是游戏设置中的20)
    float* snd_deathcamera_volume = nullptr; //死亡视角音量
    bool* snd_mute_mvp_music_live_players = nullptr; //当双方团队成员都存活时关闭MVP音乐(0--否，1--是)

    bool operator==(const SoundSettings&)const = default;
};

class C_GameSettings {
public:
    bool* cl_drawhud = nullptr;
    bool* cl_draw_only_deathnotices = nullptr;
    bool* cl_drawhud_force_radar = nullptr;
    int* cl_showfps = nullptr;
    int* cl_showtick = nullptr;
    int* cl_trueview_show_status = nullptr;
    float* host_timescale = nullptr;
    int* fps_max = nullptr;
    SoundSettings SoundSettings{};
    ScreenSettings ScreenSettings{};

    bool operator==(const C_GameSettings&)const = default;
};

class dof {
public:
    //bool指针，默认nullptr
    bool* r_dof_override = nullptr;

    float FocusDistance = 1000;
    float CrispRadius = 100;
    float BlurDistance = 100;

    //float指针，默认nullptr
    float* r_dof_override_far_blurry = nullptr;
    float* r_dof_override_far_crisp = nullptr;
    float* r_dof_override_near_blurry = nullptr;
    float* r_dof_override_near_crisp = nullptr;
    float* r_dof_override_tilt_to_ground = nullptr;
};

class GameSettingsManager final :public CSModuleBase {
private:
    C_GameSettings GameSettings{};
    dof dof{};
    bool* sv_cheats = nullptr;
    bool Menu(MulNX::UINode* node);
public:
    bool Init()override;
};