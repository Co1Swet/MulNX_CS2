#pragma once
#include <Intro/CSModuleBase.hpp>
#include <Intro/HookConsole/HookConsole.hpp>
#include <type_traits>

template<typename T>
class CvarPtr {
    const char* const name;
    T* ptr = nullptr;
public:
    constexpr explicit CvarPtr(const char* name) noexcept : name(name) {}
    void Init(HookConsole* con) {
        if (auto* cvar = con->GetCvar(this->name)) {
            this->ptr = cvar->GetPtr<T>();
        }
    }
    operator T* () const noexcept { return this->ptr; }
    T* operator->() const noexcept { return this->ptr; }
    T& operator*()  const noexcept { return *this->ptr; }
};

#define GAME_SETTINGS_CVARS(X)                          \
    /* ========== 基础 / 作弊 ========== */             \
    X(bool,  sv_cheats)                                 \
    X(int,   fps_max)                                   \
    X(float, host_timescale)                            \
                                                        \
    /* ========== 显示 / HUD ========== */              \
    X(int,   spec_show_xray)                            \
    X(int,   cl_showfps)                                \
    X(int,   cl_showtick)                               \
    X(int,   cl_trueview_show_status)                   \
    X(bool,  cl_drawhud)                                \
    X(bool,  cl_draw_only_deathnotices)                 \
    X(bool,  cl_drawhud_force_radar)                    \
    X(bool,  crosshair)                    \
                                                        \
    /* ========== 雷达 / 观战 ========== */             \
    X(bool,  cl_radar_show_all_players_when_spectating) \
    X(bool,  cl_radar_square_always)                    \
    X(bool,  cl_radar_square_when_spectating)           \
    X(bool,  cl_demo_predict)                           \
    X(bool,  cl_spec_show_bindings)                     \
                                                        \
    /* ========== 声音 ========== */                    \
    X(bool,  snd_mute_losefocus)                        \
    X(float, snd_menumusic_volume)                      \
    X(float, snd_roundstart_volume)                     \
    X(float, snd_roundaction_volume)                    \
    X(float, snd_roundend_volume)                       \
    X(float, snd_mvp_volume)                            \
    X(float, snd_mapobjective_volume)                   \
    X(float, snd_tensecondwarning_volume)               \
    X(float, snd_deathcamera_volume)                    \
    X(bool,  snd_mute_mvp_music_live_players)           \
                                                        \
    /* ========== 景深 (DoF) ========== */              \
    X(bool,  r_dof_override)                            \
    X(float, r_dof_override_far_blurry)                 \
    X(float, r_dof_override_far_crisp)                  \
    X(float, r_dof_override_near_blurry)                \
    X(float, r_dof_override_near_crisp)                 \
    X(float, r_dof_override_tilt_to_ground)

struct C_GameSettings {
#define DECL_CVAR(type, name) CvarPtr<type> name{#name};
    GAME_SETTINGS_CVARS(DECL_CVAR)
#undef DECL_CVAR
    void Init(HookConsole* cvarSys) {
#define INIT_CVAR(type, name) name.Init(cvarSys);
        GAME_SETTINGS_CVARS(INIT_CVAR)
#undef INIT_CVAR
    }
};
#undef GAME_SETTINGS_CVARS

class GameSettingsManager final :public CSModuleBase {
private:
    float FocusDistance;
    float CrispRadius;
    float BlurDistance;
    C_GameSettings settings{};
    bool Menu();
    bool SoundMenu();
    bool DofMenu();
    bool GameHudMenu();
public:
    bool Init()override;
};