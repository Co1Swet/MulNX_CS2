#include"GameCfgManager.hpp"

#include <MulNX/MulNX.hpp>
#include <MulNX/Base/UI/UI.hpp>

bool GameCfgManager::UINodeFunc(MulNX::UINode* ThisNode) {
    auto w = MulNX::UI::RAIIWindow("配置管理器", this->showWindow);
    if (!w)return true;
    //顶部工具栏
    if (ImGui::Button("刷新列表")) {
        this->UpdateCfgList();
    }
    ImGui::SameLine();
    if (ImGui::Button("关于")) {
        ImGui::OpenPopup("AboutPopup");
    }
    //关于弹窗
    if (ImGui::BeginPopup("AboutPopup")) {
        ImGui::Text("Cfg文件管理器");
        ImGui::Separator();
        ImGui::Text("功能说明:");
        ImGui::BulletText("左: 工具目录(可删除文件)");
        ImGui::BulletText("右: 游戏目录(可加载配置)");
        ImGui::EndPopup();
    }
    ImGui::Separator();
    //获取可用内容区域大小
    ImVec2 contentSize = ImGui::GetContentRegionAvail();
    const float panelHeight = contentSize.y - 20;//留出一些底部空间
    //计算列宽
    const float columnWidth = (contentSize.x - 20) * 0.5f;//减去间隔的一半
    //左侧面板 - 工具目录
    ImGui::BeginChild("工具目录", ImVec2(columnWidth, panelHeight), true, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 200, 255, 255)); //蓝色标题
    ImGui::Text("工具目录 Cfg 文件");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::Text("(%zu)", this->ToolCfgs.size());
    ImGui::Separator();
    if (this->ToolCfgs.empty()) {
        ImGui::TextDisabled("工具目录中没有cfg文件");
    }
    else {
        for (const auto& cfgName : this->ToolCfgs) {
            ImGui::PushID(("tool_" + cfgName).c_str());
            //文件项显示
            ImGui::Text("%s", cfgName.c_str());
            ImGui::SameLine(columnWidth - 170); //右对齐按钮
            //删除按钮
            if (ImGui::Button("删除", ImVec2(70, 0))) {
                if (this->DeleteCfg(cfgName)) {
                    this->UpdateCfgList();
                }
            }
            ImGui::SameLine();
            //移动到游戏目录按钮
            if (ImGui::Button("发送到游戏", ImVec2(70, 0))) {
                if (this->MoveToGame(cfgName)) {
                    this->UpdateCfgList();
                }
            }
            ImGui::PopID();
        }
    }
    ImGui::EndChild();
    ImGui::SameLine();
    //右侧面板 - 游戏目录
    ImGui::BeginChild("游戏目录", ImVec2(columnWidth, panelHeight), true, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 128, 255)); // 绿色标题
    ImGui::Text("游戏目录 Cfg 文件");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::Text("(%zu)", this->GameCfgs.size());
    ImGui::Separator();
    if (this->GameCfgs.empty()) {
        ImGui::TextDisabled("游戏目录中没有cfg文件");
    }
    else {
        for (const auto& cfgName : this->GameCfgs) {
            ImGui::PushID(("game_" + cfgName).c_str());
            //文件项显示
            ImGui::Text("%s", cfgName.c_str());
            ImGui::SameLine(columnWidth - 170);//右对齐按钮
            //加载配置按钮
            if (ImGui::Button("加载", ImVec2(70, 0))) {
                if (this->LoadCfg(cfgName)) {
                    this->LogSucc("已加载配置: " + cfgName);
                }
            }
            ImGui::SameLine();
            //移动到工具目录按钮
            if (ImGui::Button("发送到工具", ImVec2(70, 0))) {
                if (this->MoveToTool(cfgName)) {
                    this->UpdateCfgList();
                }
            }
            ImGui::PopID();
        }
    }
    ImGui::EndChild();
    //底部信息栏
    ImGui::Separator();
    ImGui::TextDisabled("提示: 点击按钮在工具目录和游戏目录间移动文件");
    return true;
}

bool GameCfgManager::Init() {
    //基础服务
	this->IPCer = this->Core->ModuleManager()->FindModule<MulNX::IPCer>("IPCer");
    //路径绑定
    this->ToolPath = this->PathGet("CS2Configs");
    this->GamePath = this->CS2Paths->config;
	//初始化Cfg文件列表
    this->UpdateCfgList();
    this->SendUIRoot(this->GetName(), [this](MulNX::UINode* node) {return this->UINodeFunc(node);});
    return true;
}

bool GameCfgManager::UpdateCfgList() {
	//清空现有列表
	this->GameCfgs.clear();
	this->ToolCfgs.clear();
	//获取游戏目录Cfg文件列表
	if (!this->IPCer->GetFileNames(this->GameCfgs, this->GamePath, std::vector<std::string>{".cfg"}, false)) {
		this->LogError("获取游戏目录Cfg文件列表失败");
		return false;
	}
	//获取工具目录Cfg文件列表
	if (!this->IPCer->GetFileNames(this->ToolCfgs, this->ToolPath, std::vector<std::string>{".cfg"}, false)) {
		this->LogError("获取工具目录Cfg文件列表失败");
		return false;
	}
	//对两个列表进行排序，便于查看
	std::sort(this->GameCfgs.begin(), this->GameCfgs.end());
	std::sort(this->ToolCfgs.begin(), this->ToolCfgs.end());

	this->LogSucc("Cfg文件列表更新完成");
	return true;
}
bool GameCfgManager::MoveToGame(const std::string& CfgName) {
    const std::filesystem::path CfgPath = this->ToolPath / (CfgName + ".cfg");
    const std::string FullName = CfgName + ".cfg";
    if (!this->IPCer->FileMove(FullName, this->ToolPath, this->GamePath)) {
		this->LogError("从工具目录移动到游戏目录失败，文件可能不存在或移动过程中出现错误！  路径：" + CfgPath.string());
		return false;
	}
	return true;
}
bool GameCfgManager::LoadCfg(const std::string& CfgName) {
	const std::filesystem::path CfgPath = this->GamePath / (CfgName + ".cfg");
	if (!std::filesystem::exists(CfgPath)) {
		this->LogError("指定的配置文件不存在，无法加载配置文件！  路径：" + CfgPath.string());
		return false;
	}
	this->AsyncCommand("exec " + CfgName);
	this->LogSucc("成功加载配置文件，路径：" + CfgPath.string());
	return true;
}
bool GameCfgManager::MoveToTool(const std::string& CfgName) {
    const std::filesystem::path CfgPath = this->ToolPath / (CfgName + ".cfg");
    const std::string FullName = CfgName + ".cfg";
    if (!this->IPCer->FileMove(FullName, this->GamePath, this->ToolPath)) {
		this->LogError("从游戏目录移动到工具目录失败，文件可能不存在或移动过程中出现错误！  路径：" + CfgPath.string());
		return false;
	}
	return true;
}
bool GameCfgManager::DeleteCfg(const std::string& CfgName) {
    const std::filesystem::path CfgPath = this->ToolPath / (CfgName + ".cfg");
    const std::string FullName = CfgName + ".cfg";
    if (!this->IPCer->FileDelete(FullName, this->ToolPath)) {
		this->LogError("从工具目录删除配置文件失败，文件可能不存在或删除过程中出现错误！  路径：" + CfgPath.string());
		return false;
	}
	return true;
}