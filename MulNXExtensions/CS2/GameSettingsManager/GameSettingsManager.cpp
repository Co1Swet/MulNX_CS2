#include "GameSettingsManager.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CS2/CSController/CSController.hpp>

bool GameSettingsManager::Menu(MulNX::UINode* ThisNode) {
    ImGui::Checkbox("作弊模式", this->sv_cheats);

    ImGui::SliderFloat("游戏速度", this->GameSettings.host_timescale, 0.001f, 10.000f);

    if (ImGui::CollapsingHeader("画面设置")) {
        ImGui::SliderInt("FPS上限", this->GameSettings.fps_max, 0, 1000);

        if (ImGui::TreeNode("UI设置")) {
            ImGui::Checkbox("显示HUD", this->GameSettings.cl_drawhud);
            ImGui::Checkbox("只渲染击杀信息", this->GameSettings.cl_draw_only_deathnotices);
            ImGui::Checkbox("强制雷达渲染", this->GameSettings.cl_drawhud_force_radar);
            ImGui::SliderInt("展示FPS", this->GameSettings.cl_showfps, 0, 3, "%d");
            ImGui::SliderInt("展示Tick", this->GameSettings.cl_showtick, 0, 3, "%d");
            ImGui::SliderInt("TrueView控制", this->GameSettings.cl_trueview_show_status, 0, 2);
            if (ImGui::Button("切换Demo进度条UI显示")) {
                this->ISys().AsyncCommand("demoui");
            }

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("渲染设置")) {
            ImGui::SliderInt("X光", this->GameSettings.ScreenSettings.spec_show_xray, 0, 100);
            if (ImGui::TreeNode("景深控制")) {

                ImGui::Checkbox("启用景深", this->dof.r_dof_override);

                bool DOFChange = false;
                DOFChange |= ImGui::SliderFloat("聚焦距离", &this->dof.FocusDistance, 0, 5000);
                DOFChange |= ImGui::SliderFloat("清晰半径", &this->dof.CrispRadius, 0, 5000);
                DOFChange |= ImGui::SliderFloat("模糊距离", &this->dof.BlurDistance, 0, 5000);
                if (DOFChange) {
                    MulNX::Math::DOFParam Param;
                    MulNX::Math::CalculateDOFParameters(this->dof.FocusDistance, this->dof.CrispRadius, this->dof.BlurDistance, Param);

                    *this->dof.r_dof_override_near_blurry = Param.NearBlurry;// 近模糊
                    *this->dof.r_dof_override_near_crisp = Param.NearCrisp;// 近清晰
                    *this->dof.r_dof_override_far_crisp = Param.FarCrisp;// 远清晰
                    *this->dof.r_dof_override_far_blurry = Param.FarBlurry;// 远模糊
                }

                ImGui::Separator();

                ImGui::SliderFloat("r_dof_override_far_blurry", this->dof.r_dof_override_far_blurry, 0, 5000);
                ImGui::SliderFloat("r_dof_override_far_crisp", this->dof.r_dof_override_far_crisp, 0, 5000);
                ImGui::SliderFloat("r_dof_override_near_crisp", this->dof.r_dof_override_near_crisp, 0, 5000);
                ImGui::SliderFloat("r_dof_override_near_blurry", this->dof.r_dof_override_near_blurry, 0, 5000);

                ImGui::SliderFloat("r_dof_override_tilt_to_ground", this->dof.r_dof_override_tilt_to_ground, 0, 5000);

                ImGui::TreePop();
            }
            ImGui::TreePop();
        }
    }

    if (ImGui::CollapsingHeader("声音设置")) {
        ImGui::Checkbox("游戏窗口失去焦点时静音", this->GameSettings.SoundSettings.snd_mute_losefocus);

        if (ImGui::TreeNode("音乐设置")) {
            ImGui::SliderFloat("主菜单音量", this->GameSettings.SoundSettings.snd_menumusic_volume, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("回合开始音量", this->GameSettings.SoundSettings.snd_roundstart_volume, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("回合开始行动音量", this->GameSettings.SoundSettings.snd_roundaction_volume, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("回合结束音量", this->GameSettings.SoundSettings.snd_roundend_volume, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("MVP音量", this->GameSettings.SoundSettings.snd_mvp_volume, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("炸弹/人质音量", this->GameSettings.SoundSettings.snd_mapobjective_volume, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("十秒警告音量", this->GameSettings.SoundSettings.snd_tensecondwarning_volume, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("死亡视角音量", this->GameSettings.SoundSettings.snd_deathcamera_volume, 0.0f, 1.0f, "%.2f");
            ImGui::Checkbox("当双方团队成员都存活时关闭MVP音乐", this->GameSettings.SoundSettings.snd_mute_mvp_music_live_players);

            ImGui::TreePop();
        }
        if (ImGui::TreeNode("音效设置")) {

            ImGui::TreePop();
        }
    }

    return true;
}

bool GameSettingsManager::Init() {
    this->SendTask("CSControl", [this]() {
        C_ConVarSystem& CVarSystem = this->CS2()->GetCvarSystem();

        auto spec_show_xray = CVarSystem.GetCvar("spec_show_xray");
        if (!spec_show_xray)return true;

        this->GameSettings.ScreenSettings.spec_show_xray = spec_show_xray->GetPtr<int>();

        this->dof.r_dof_override = CVarSystem.GetCvar("r_dof_override")->GetPtr<bool>();
        this->dof.r_dof_override_far_blurry = CVarSystem.GetCvar("r_dof_override_far_blurry")->GetPtr<float>();
        this->dof.r_dof_override_far_crisp = CVarSystem.GetCvar("r_dof_override_far_crisp")->GetPtr<float>();
        this->dof.r_dof_override_near_blurry = CVarSystem.GetCvar("r_dof_override_near_blurry")->GetPtr<float>();
        this->dof.r_dof_override_near_crisp = CVarSystem.GetCvar("r_dof_override_near_crisp")->GetPtr<float>();
        this->dof.r_dof_override_tilt_to_ground = CVarSystem.GetCvar("r_dof_override_tilt_to_ground")->GetPtr<float>();

        this->GameSettings.cl_drawhud = CVarSystem.GetCvar("cl_drawhud")->GetPtr<bool>();
        this->GameSettings.cl_draw_only_deathnotices = CVarSystem.GetCvar("cl_draw_only_deathnotices")->GetPtr<bool>();
        this->GameSettings.cl_drawhud_force_radar = CVarSystem.GetCvar("cl_drawhud_force_radar")->GetPtr<bool>();
        this->GameSettings.cl_showfps = CVarSystem.GetCvar("cl_showfps")->GetPtr<int>();
        this->GameSettings.cl_showtick = CVarSystem.GetCvar("cl_showtick")->GetPtr<int>();
        this->GameSettings.cl_trueview_show_status = CVarSystem.GetCvar("cl_trueview_show_status")->GetPtr<int>();
        this->GameSettings.host_timescale = CVarSystem.GetCvar("host_timescale")->GetPtr<float>();
        this->GameSettings.fps_max = CVarSystem.GetCvar("fps_max")->GetPtr<int>();

        this->sv_cheats = CVarSystem.GetCvar("sv_cheats")->GetPtr<bool>();

        this->GameSettings.SoundSettings.snd_mute_losefocus = CVarSystem.GetCvar("snd_mute_losefocus")->GetPtr<bool>();
        this->GameSettings.SoundSettings.snd_menumusic_volume = CVarSystem.GetCvar("snd_menumusic_volume")->GetPtr<float>();
        this->GameSettings.SoundSettings.snd_roundstart_volume = CVarSystem.GetCvar("snd_roundstart_volume")->GetPtr<float>();
        this->GameSettings.SoundSettings.snd_roundaction_volume = CVarSystem.GetCvar("snd_roundaction_volume")->GetPtr<float>();
        this->GameSettings.SoundSettings.snd_roundend_volume = CVarSystem.GetCvar("snd_roundend_volume")->GetPtr<float>();
        this->GameSettings.SoundSettings.snd_mvp_volume = CVarSystem.GetCvar("snd_mvp_volume")->GetPtr<float>();
        this->GameSettings.SoundSettings.snd_mapobjective_volume = CVarSystem.GetCvar("snd_mapobjective_volume")->GetPtr<float>();
        this->GameSettings.SoundSettings.snd_tensecondwarning_volume = CVarSystem.GetCvar("snd_tensecondwarning_volume")->GetPtr<float>();
        this->GameSettings.SoundSettings.snd_deathcamera_volume = CVarSystem.GetCvar("snd_deathcamera_volume")->GetPtr<float>();
        this->GameSettings.SoundSettings.snd_mute_mvp_music_live_players = CVarSystem.GetCvar("snd_mute_mvp_music_live_players")->GetPtr<bool>();

        *this->GameSettings.cl_trueview_show_status = 0;
        this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {return this->Menu(node);});
        return false;
        });

    return true;
}