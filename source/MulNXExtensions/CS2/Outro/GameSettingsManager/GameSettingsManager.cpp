#include "GameSettingsManager.hpp"

bool GameSettingsManager::Window() {
    auto w = MulNX::UI::RAIIWindow(I18n("ui.game_settings").c_str());
    
    ConvarCheckbox<"sv_cheats">("作弊模式");
    ConvarSliderFloat<"fps_max">("FPS上限", 0, 1000);
    ConvarSliderFloat<"host_timescale">("游戏速度", 0.001f, 10.000f);
    ConvarCheckbox<"cl_demo_predict">("真实视角");

    ImGui::SeparatorText("CS2控制台");
    if (ImGui::Button("解限所有CS2控制台变量")) {
        int Count = 0;
        this->CS2Con->UnlockHiddenCVars(Count);
        this->LogSucc("成功解限" + std::to_string(Count) + "个控制台命令！");
    }
    if (ImGui::Button("限住所有CS2控制台变量")) {
        int Count = 0;
        this->CS2Con->LockAllCvars(Count);
        this->LogSucc("成功限住" + std::to_string(Count) + "个控制台命令！");
    }
    if (ImGui::Button("列出所有CS2控制台变量")) {
        this->LogLine();
        uint64_t idx = 0;
        this->CS2Con->GetFirstCvarIterator(idx);
        while (idx != 0xFFFFFFFF) {
            C_ConVar* var = this->CS2Con->GetCVarByIndex(idx);
            if (var) {
                std::string Name = var->szName ? var->szName : "未知";
                this->LogInfo("控制台命令：" + Name);
            }
            this->CS2Con->GetNextCvarIterator(idx, idx);
        }
        this->LogLine();
    }

    return true;
}

bool GameSettingsManager::SoundMenu() {
    ConvarCheckbox<"snd_mute_losefocus">("游戏窗口失去焦点时静音");

    ImGui::SeparatorText("音乐设置");
    ConvarSliderFloat<"snd_menumusic_volume">("主菜单音量", 0.0f, 1.0f);
    ConvarSliderFloat<"snd_roundstart_volume">("回合开始音量", 0.0f, 1.0f);
    ConvarSliderFloat<"snd_roundaction_volume">("回合开始行动音量", 0.0f, 1.0f);
    ConvarSliderFloat<"snd_roundend_volume">("回合结束音量", 0.0f, 1.0f);
    ConvarSliderFloat<"snd_mvp_volume">("MVP音量", 0.0f, 1.0f);
    ConvarSliderFloat<"snd_mapobjective_volume">("炸弹/人质音量", 0.0f, 1.0f);
    ConvarSliderFloat<"snd_tensecondwarning_volume">("十秒警告音量", 0.0f, 1.0f);
    ConvarSliderFloat<"snd_deathcamera_volume">("死亡视角音量", 0.0f, 1.0f);
    ConvarCheckbox<"snd_mute_mvp_music_live_players">("当双方团队成员都存活时关闭MVP音乐");

    return true;
}

bool GameSettingsManager::DofMenu() {
    ImGui::SeparatorText("景深控制");
    ConvarCheckbox<"r_dof_override">("启用景深");

    bool DOFChange = false;
    DOFChange |= ImGui::SliderFloat("聚焦距离", &this->FocusDistance, 0, 5000);
    DOFChange |= ImGui::SliderFloat("清晰半径", &this->CrispRadius, 0, 5000);
    DOFChange |= ImGui::SliderFloat("模糊距离", &this->BlurDistance, 0, 5000);
    if (DOFChange) {
        MulNX::Math::DOFParam Param;
        MulNX::Math::CalculateDOFParameters(this->FocusDistance, this->CrispRadius, this->BlurDistance, Param);

        *this->r_dof_override_near_blurry = Param.NearBlurry;// 近模糊
        *this->r_dof_override_near_crisp = Param.NearCrisp;// 近清晰
        *this->r_dof_override_far_crisp = Param.FarCrisp;// 远清晰
        *this->r_dof_override_far_blurry = Param.FarBlurry;// 远模糊
    }

    ImGui::SeparatorText("调整上方参数，自动计算并应用以下参数：");

    ImGui::SliderFloat("r_dof_override_far_blurry", this->r_dof_override_far_blurry, 0, 5000);
    ImGui::SliderFloat("r_dof_override_far_crisp", this->r_dof_override_far_crisp, 0, 5000);
    ImGui::SliderFloat("r_dof_override_near_crisp", this->r_dof_override_near_crisp, 0, 5000);
    ImGui::SliderFloat("r_dof_override_near_blurry", this->r_dof_override_near_blurry, 0, 5000);
    ImGui::Separator();
    ConvarSliderFloat<"r_dof_override_tilt_to_ground">("r_dof_override_tilt_to_ground", 0, 5000);

    return true;
}

bool GameSettingsManager::GameHudMenu() {
    ImGui::SeparatorText("游戏HUD设置");

    ConvarCheckbox<"cl_drawhud">("显示HUD");
    ConvarCheckbox<"cl_draw_only_deathnotices">("只渲染击杀信息");
    ConvarSliderInt<"cl_showfps">("展示FPS", 0, 3);
    ConvarSliderInt<"cl_showtick">("展示Tick", 0, 3);
    ConvarSliderInt<"cl_trueview_show_status">("TrueView控制", 0, 2);
    ConvarSliderInt<"spec_show_xray">("观战X光", 0, 100);

    ConvarCheckbox<"cl_drawhud_force_radar">("强制雷达渲染");
    ConvarCheckbox<"cl_radar_square_when_spectating">("在观战时使用方形雷达");
    ConvarCheckbox<"cl_radar_square_always">("永远使用方形雷达");
    ConvarCheckbox<"cl_radar_show_all_players_when_spectating">("雷达在观战时显示所有人");

    ConvarCheckbox<"crosshair">("准星");
    if (ImGui::Button("切换Demo进度条UI显示")) {
        this->AsyncCommand("demoui");
    }
    return true;
}

bool GameSettingsManager::Init() {
    this->SubscribeSync("Hook/Source2Client002::Inited", [this](MulNX::Message& msg) {

        this->r_dof_override_near_blurry = this->CS2Con->GetCvar("r_dof_override_near_blurry")->GetPtr<float>();
        this->r_dof_override_near_crisp = this->CS2Con->GetCvar("r_dof_override_near_crisp")->GetPtr<float>();
        this->r_dof_override_far_crisp = this->CS2Con->GetCvar("r_dof_override_far_crisp")->GetPtr<float>();
        this->r_dof_override_far_blurry = this->CS2Con->GetCvar("r_dof_override_far_blurry")->GetPtr<float>();

        *this->CS2Con->GetCvar("cl_trueview_show_status")->GetPtr<int>() = 0;
        *this->CS2Con->GetCvar("cl_radar_show_all_players_when_spectating")->GetPtr<bool>() = false;
        *this->CS2Con->GetCvar("cl_radar_square_always")->GetPtr<bool>() = false;
        *this->CS2Con->GetCvar("cl_radar_square_when_spectating")->GetPtr<bool>() = false;
        *this->CS2Con->GetCvar("cl_demo_predict")->GetPtr<bool>() = false;
        *this->CS2Con->GetCvar("cl_spec_show_bindings")->GetPtr<bool>() = false;

        this->SendUIRoot(this->GetName(), [this](auto&&...) {return this->Window();});
        this->UIRegisterCallback("UI.Sound", [this](auto&&...) {return this->SoundMenu();});
        this->UIRegisterCallback("UI.CameraSetting", [this](auto&&...) {return this->DofMenu();});
        this->UIRegisterCallback("UI.2DVision", [this](auto&&...) {return this->GameHudMenu();});
        return false;
        });

    return true;
}