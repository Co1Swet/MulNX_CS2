#include "ConsoleManager.hpp"

#include <MulNX/MulNX.hpp>
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CS2/CSController/CSController.hpp>

bool ConsoleManager::Init() {
    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {return this->UINodeFunc(node);});
    return true;
}

bool ConsoleManager::UINodeFunc(MulNX::UINode* node) {
    node->CallUINode("GraphicsManager");
    MulNX::UI::Checkbox("小地图窗口", this->Core->ModuleManager()->FindModule("MiniMap")->showWindow);
    MulNX::UI::Checkbox("游戏配置管理器窗口", this->Core->ModuleManager()->FindModule("GameCfgManager")->showWindow);
    MulNX::UI::Checkbox("快捷操作窗口", this->Core->ModuleManager()->FindModule("CSController")->showWindow);
    MulNX::UI::Checkbox("Demo", this->Core->ModuleManager()->FindModule("DemoSystem")->showWindow);
    MulNX::UI::Checkbox("玩家信息管理窗口", this->Core->ModuleManager()->FindModule("PlayerHub")->showWindow);
    MulNX::UI::Checkbox("投掷物追踪器窗口", this->Core->ModuleManager()->FindModule("ProjectileTracker")->showWindow);
    MulNX::UI::Checkbox(I18n("dthmsg.window.control").c_str(), this->Core->ModuleManager()->FindModule("DeathMsgController")->showWindow);
    MulNX::UI::Checkbox("ESP", this->Core->ModuleManager()->FindModule("ESPController")->showWindow);

    if (ImGui::CollapsingHeader("CS2控制台")) {
        if (ImGui::Button("解限所有CS2控制台变量")) {
            int Count = 0;
            this->CS2()->GetCvarSystem().UnlockHiddenCVars(Count);
            this->ISys().LogSucc("成功解限" + std::to_string(Count) + "个控制台命令！");
        }
        if (ImGui::Button("限住所有CS2控制台变量")) {
            int Count = 0;
            this->CS2()->GetCvarSystem().LockAllCvars(Count);
            this->ISys().LogSucc("成功限住" + std::to_string(Count) + "个控制台命令！");
        }
        if (ImGui::Button("列出所有CS2控制台变量")) {
            this->ISys().LogLine();
            uint64_t idx = 0;
            this->CS2()->GetCvarSystem().GetFirstCvarIterator(idx);
            while (idx != 0xFFFFFFFF) {
                C_ConVar* var = this->CS2()->GetCvarSystem().GetCVarByIndex(idx);
                if (var) {
                    std::string Name = var->szName ? var->szName : "未知";
                    this->ISys().LogInfo("控制台命令：" + Name);
                }
                this->CS2()->GetCvarSystem().GetNextCvarIterator(idx);
            }
            this->ISys().LogLine();
        }
    }
    return true;
}