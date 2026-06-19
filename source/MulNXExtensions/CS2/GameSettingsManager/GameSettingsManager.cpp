#include "GameSettingsManager.hpp"
#include <MulNX/Base/UI/UI.hpp>

bool GameSettingsManager::Menu(MulNX::UINode* ThisNode) {
    ImGui::Checkbox("作弊模式", this->settings.sv_cheats);
    ImGui::SliderInt("FPS上限", this->settings.fps_max, 0, 1000);
    ImGui::SliderFloat("游戏速度", this->settings.host_timescale, 0.001f, 10.000f);
    ImGui::Checkbox("真实视角", this->settings.cl_demo_predict);

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

bool GameSettingsManager::SoundMenu(MulNX::UINode* node) {
    ImGui::Checkbox("游戏窗口失去焦点时静音", this->settings.snd_mute_losefocus);

    ImGui::SeparatorText("音乐设置");
    ImGui::SliderFloat("主菜单音量", this->settings.snd_menumusic_volume, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("回合开始音量", this->settings.snd_roundstart_volume, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("回合开始行动音量", this->settings.snd_roundaction_volume, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("回合结束音量", this->settings.snd_roundend_volume, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("MVP音量", this->settings.snd_mvp_volume, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("炸弹/人质音量", this->settings.snd_mapobjective_volume, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("十秒警告音量", this->settings.snd_tensecondwarning_volume, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("死亡视角音量", this->settings.snd_deathcamera_volume, 0.0f, 1.0f, "%.2f");
    ImGui::Checkbox("当双方团队成员都存活时关闭MVP音乐", this->settings.snd_mute_mvp_music_live_players);

    return true;
}

bool GameSettingsManager::DofMenu(MulNX::UINode* node) {
    ImGui::SeparatorText("景深控制");
    ImGui::Checkbox("启用景深", this->settings.r_dof_override);

    bool DOFChange = false;
    DOFChange |= ImGui::SliderFloat("聚焦距离", &this->FocusDistance, 0, 5000);
    DOFChange |= ImGui::SliderFloat("清晰半径", &this->CrispRadius, 0, 5000);
    DOFChange |= ImGui::SliderFloat("模糊距离", &this->BlurDistance, 0, 5000);
    if (DOFChange) {
        MulNX::Math::DOFParam Param;
        MulNX::Math::CalculateDOFParameters(this->FocusDistance, this->CrispRadius, this->BlurDistance, Param);

        *this->settings.r_dof_override_near_blurry = Param.NearBlurry;// 近模糊
        *this->settings.r_dof_override_near_crisp = Param.NearCrisp;// 近清晰
        *this->settings.r_dof_override_far_crisp = Param.FarCrisp;// 远清晰
        *this->settings.r_dof_override_far_blurry = Param.FarBlurry;// 远模糊
    }

    ImGui::SeparatorText("调整上方参数，自动计算并应用以下参数：");

    ImGui::SliderFloat("r_dof_override_far_blurry", this->settings.r_dof_override_far_blurry, 0, 5000);
    ImGui::SliderFloat("r_dof_override_far_crisp", this->settings.r_dof_override_far_crisp, 0, 5000);
    ImGui::SliderFloat("r_dof_override_near_crisp", this->settings.r_dof_override_near_crisp, 0, 5000);
    ImGui::SliderFloat("r_dof_override_near_blurry", this->settings.r_dof_override_near_blurry, 0, 5000);
    ImGui::Separator();
    ImGui::SliderFloat("r_dof_override_tilt_to_ground", this->settings.r_dof_override_tilt_to_ground, 0, 5000);

    return true;
}

bool GameSettingsManager::GameHudMenu(MulNX::UINode* node) {
    ImGui::SeparatorText("游戏HUD设置");

    ImGui::Checkbox("显示HUD", this->settings.cl_drawhud);
    ImGui::Checkbox("只渲染击杀信息", this->settings.cl_draw_only_deathnotices);
    ImGui::Checkbox("强制雷达渲染", this->settings.cl_drawhud_force_radar);
    ImGui::SliderInt("展示FPS", this->settings.cl_showfps, 0, 3, "%d");
    ImGui::SliderInt("展示Tick", this->settings.cl_showtick, 0, 3, "%d");
    ImGui::SliderInt("TrueView控制", this->settings.cl_trueview_show_status, 0, 2);
    ImGui::SliderInt("X光", this->settings.spec_show_xray, 0, 100);
    ImGui::Checkbox("方形雷达", this->settings.cl_radar_square_when_spectating);
    ImGui::Checkbox("准星", this->settings.crosshair);
    if (ImGui::Button("切换Demo进度条UI显示")) {
        this->AsyncCommand("demoui");
    }
    return true;
}

bool GameSettingsManager::Init() {
    this->SubscribeSync("Hook/Source2Client002::Inited", [this](MulNX::Message& msg) {
        this->settings.Init(this->CS2Con);

        *this->settings.cl_trueview_show_status = 0;
        *this->settings.cl_radar_show_all_players_when_spectating = false;
        *this->settings.cl_radar_square_always = false;
        *this->settings.cl_radar_square_when_spectating = false;
        *this->settings.cl_demo_predict = false;
        *this->settings.cl_spec_show_bindings = false;

        this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {return this->Menu(node);});
        this->SendUINode("SoundMenu", [this](MulNX::UINode* node) {return this->SoundMenu(node);});
        this->SendUINode("DofMenu", [this](MulNX::UINode* node) {return this->DofMenu(node);});
        this->SendUINode("GameHudMenu", [this](MulNX::UINode* node) {return this->GameHudMenu(node);});
        return false;
        });

    return true;
}