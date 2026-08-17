#include "SolutionManager.hpp"
#include <Buildup/TimeController/TimeController.hpp>
#include <Intro/HookView/HookView.hpp>
#include <CameraSystem/CameraSystem.hpp>
#include <CameraSystem/CameraDrawer/CameraDrawer.hpp>
#include <CameraSystem/ElementManager/ElementManager.hpp>
#include <CameraSystem/ProjectManager/ProjectManager.hpp>

bool SolutionManager::MenuSolution() {
    ImGui::Text(I18n(
        "camsys.sol.play_status",
        this->Playing ? I18n("text.opened") : I18n("text.closed"),
        this->Playing_pSolution ? this->Playing_pSolution->GetName() : I18n("text.none")
    ).c_str());
    ImGui::Separator();
    // 解决方案总设置
    if (ImGui::CollapsingHeader(I18n("camsys.sol.settings").c_str())) {
        ImGui::Checkbox(I18n("camsys.sol.shortcut_enable").c_str(), &this->Config.SolutionShortcutEnable);
        ImGui::Checkbox(I18n("camsys.sol.playing_draw").c_str(), &this->Config.PlayingDraw);
        ImGui::Checkbox(I18n("camsys.sol.playing_override").c_str(), &this->Config.PlayingOverride);
    }
    // 创建解决方案
    if (ImGui::CollapsingHeader(I18n("camsys.sol.create").c_str())) {
        ImGui::Text(I18n("camsys.sol.new_name").c_str());
        ImGui::SameLine();
        static std::string CreateSolutionName = "";
        ImGui::InputText("##新解决方案名", &CreateSolutionName);
        ImGui::SameLine();
        // 创建成功则清空输入框
        if (ImGui::Button(I18n("camsys.sol.create_btn").c_str())) {
            if (CreateSolutionName.empty()) {
                this->LogError(I18n("result.error_empty_name").c_str());
                return true;
            }
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CameraSystem/Solution/Create"_hash);
            rp->str1 = std::move(CreateSolutionName);
            this->PublishAsync(std::move(msg));
            CreateSolutionName.clear();
        }
    }
    // 展示修改解决方案
    if (ImGui::CollapsingHeader(I18n("camsys.sol.list").c_str())) {
        for (const auto& [name, solution] : this->solutions) {
            this->Solution_ShowInLine(solution.get());
        }
    }

    return true;
}
void SolutionManager::Solution_ShowInLine(Solution* solution) {
    ImGui::Text(I18n("camsys.sol.name_label").c_str());
    ImGui::SameLine();
    if (ImGui::Selectable(solution->name.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
        if (ImGui::IsMouseDoubleClicked(0)) {
            this->CurrentSolution = solution;
            this->showWindow.store(true, std::memory_order_release);
        }
    }
    if (ImGui::BeginPopupContextItem(("右键菜单" + solution->name).c_str())) {
        if (ImGui::MenuItem(I18n("text.copy_name").c_str())) {
            ImGui::SetClipboardText(solution->name.c_str());
        }
        if (ImGui::MenuItem(I18n("text.save").c_str())) {
            auto path = this->Path()->PathGetFromKey("Solutions");
            auto [ok, msg] = solution->Save(path);
            if (ok) {
                this->LogSucc(std::move(msg));
            }
            else {
                this->LogError(std::move(msg));
            }
        }
        if (ImGui::MenuItem(I18n("text.print_debug").c_str())) {
            this->LogLine();
            this->LogInfo(solution->GetMsg());
            this->LogLine();
        }
        if (ImGui::MenuItem(I18n("text.delete").c_str())) {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CameraSystem/Solution/Delete"_hash);
            rp->str1 = solution->name;
            this->PublishAsync(std::move(msg));
        }
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    ImGui::Text(I18n("camsys.sol.element_count_duration",
        solution->elements.size(),
        solution->totalDurationTime
    ).c_str());
}
bool SolutionManager::UINodeFunc() {
    if (this->needDrawCamera.load(std::memory_order_acquire) && this->Config.PlayingDraw) {
        auto frame = this->drawCamera.Read();
        this->CamDrawer->DrawFrameCamera(*frame, I18n("camsys.sol.playing_draw_label").c_str());
    }

    if (!this->showWindow.load(std::memory_order_acquire))return true;
    this->Solution_DebugWindow();
    if (!this->CurrentSolution) {
        this->OpenSolutionKCPackDebugWindow = false;
    }
    if (!this->OpenSolutionKCPackDebugWindow)return true;
    if (this->Buffer_KCPack.DebugWindow(this->OpenSolutionKCPackDebugWindow)) {
        this->OpenSolutionKCPackDebugWindow.store(false, std::memory_order_release);
        if (!this->Buffer_KCPack.Usable) {
            this->LogError("当前按键绑定不可用，无法使用这个绑键播放解决方案！");
        }
        else {
            this->CurrentSolution->KCPack = this->Buffer_KCPack;//更新绑键
        }
    }
    return true;
}
void SolutionManager::Solution_DebugWindow() {
    auto w = MulNX::UI::RAIIWindow(I18n("camsys.sol.debug_window").c_str(), this->showWindow);
    if (!w || !w.ShouldDraw())return;
    // 检查当前是否操作解决方案
    if (!this->CurrentSolution) {
        ImGui::Text(I18n("ui.button.no_selected").c_str());
        return;
    }

    ImGui::Text(I18n("camsys.sol.current_info",
        this->CurrentSolution->name,
        this->CurrentSolution->elements.size(),
        this->CurrentSolution->totalDurationTime,
        PlaybackModeToString(this->CurrentSolution->playmode)
    ).c_str());
    if (ImGui::Button(I18n("camsys.sol.switch_to_activation").c_str())) {
        this->CurrentSolution->playmode = PlaybackMode::Activation;
    }
    ImGui::SameLine();
    if (ImGui::Button(I18n("camsys.sol.switch_to_orchestration").c_str())) {
        this->CurrentSolution->playmode = PlaybackMode::Orchestration;
    }
    if (ImGui::Button(I18n("camsys.sol.enable_current").c_str())) {
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CameraSystem/Solution/Play"_hash);
        rp->str1 = this->CurrentSolution->name;
        this->PublishAsync(std::move(msg));
    }
    ImGui::SameLine();
    // if (ImGui::Button("按激活模式生成编排模式偏移")) {
    //     this->CurrentSolution->TimeLineGenerate();
    // }
    if (ImGui::Button(I18n("text.modify_keybind").c_str())) {
        this->Buffer_KCPack = this->CurrentSolution->KCPack;//缓存
        this->OpenSolutionKCPackDebugWindow = true;//打开窗口
    }
    ImGui::Separator();

    static std::string NewElementName = "";
    ImGui::InputText(I18n("camsys.sol.new_element_name").c_str(), &NewElementName);
    if (ImGui::Button(I18n("text.add").c_str())) {
        auto it = this->EManager->elements.find(NewElementName);
        if (it == this->EManager->elements.end()) {
            this->LogError("找不到目标元素   元素名：" + NewElementName);
        }
        else {
            if (!this->CurrentSolution->AddElement(it->second, 0)) {
                this->LogError("无法添加元素到解决方案，可能是元素已存在于解决方案中   元素名：" + NewElementName);
            }
            else {
                this->LogSucc("成功添加元素到解决方案   元素名：" + NewElementName);
                NewElementName.clear();
            }
        }
    }
    ImGui::SameLine();
    if (ImGui::Button(I18n("text.clear").c_str())) {
        this->CurrentSolution->Clear();
        this->LogSucc("成功清空解决方案所有元素");
    }

    ImGui::Separator();

    static int IndexForReset = 0;
    static int PreIndex = -1;
    ImGui::SliderInt(I18n("camsys.sol.adjust_element_index").c_str(), &IndexForReset, 0, this->CurrentSolution->elements.size() - 1);
    if (this->CurrentSolution->elements.empty())return;
    std::shared_ptr<ElementBase> element = this->CurrentSolution->elements.at(IndexForReset).Element;
    if (element) {
        const float& Offset = this->CurrentSolution->elements.at(IndexForReset).Offset;
        ImGui::Text(I18n("camsys.sol.element_info",
            IndexForReset, element->Name, element->DurationTime, Offset).c_str());
        ImGui::Separator();
        static float tempOffset{};
        if (IndexForReset != PreIndex) {
            tempOffset = Offset;
        }
        ImGui::SliderFloat(I18n("camsys.sol.offset_time").c_str(), &tempOffset, 0, 100000);
        if (ImGui::Button(I18n("text.confirm_modify").c_str())) {
            this->CurrentSolution->RemoveElementAt(IndexForReset);
            this->CurrentSolution->AddElement(element, tempOffset);
        }
        if (ImGui::Button(I18n("text.remove").c_str())) {
            this->CurrentSolution->RemoveElementAt(IndexForReset);
        }
    }
    PreIndex = IndexForReset;
}

//解决方案管理器

bool SolutionManager::Init() {
    this->CamDrawer = &this->Core->ModuleManager()->FindModule<CameraSystem>("CameraSystem")->CamDrawer;
    this->EManager = this->Core->ModuleManager()->FindModule<ElementManager>("ElementManager");
    this->PManager = this->Core->ModuleManager()->FindModule<ProjectManager>("ProjectManager");

    this->SendUIRoot(this->GetName(), [this](auto&&...) {return this->UINodeFunc();});

    auto* PathManager = this->Path();
    if (PathManager->CreateKey("Solutions", "Solutions",
        [this](MulNX::PathManager* PathManager)->bool {
            auto Path = PathManager->PathGetFromKey("Solutions");
            this->LogSucc("成功设置解决方案路径为：" + Path.string());
            return true;
        })) {
        PathManager->KeyBindDynamic("Solutions", "CurrentProject");
    }
    (*this)
        .SubscribeAsync("CameraSystem/Element/Deleted")
        .SubscribeAsync("CameraSystem/Solution/Create")
        .SubscribeAsync("CameraSystem/Solution/Delete")
        .SubscribeAsync("CameraSystem/Solution/Play");

    return true;
}

void SolutionManager::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "CameraSystem/Solution/Create"_hash: {
        auto name = msg.asp.get<MulNX::NetExt>()->str1;
        std::unique_lock lock(this->CamSys->smutex);
        if (!this->Solution_Create(name)) {
            this->LogError(std::format("创建解决方案失败：{}", name));
        }
        break;
    }
    case "CameraSystem/Solution/Delete"_hash: {
        auto name = msg.asp.get<MulNX::NetExt>()->str1;
        std::unique_lock lock(this->CamSys->smutex);
        if (!this->Solution_Delete(name)) {
            this->LogError(std::format("删除解决方案失败：{}", name));
        }
        break;
    }
    case "CameraSystem/Solution/Play"_hash: {
        auto name = msg.asp.get<MulNX::NetExt>()->str1;
        std::unique_lock lock(this->CamSys->smutex);
        this->Playing_Solution(name);
        break;
    }
    case "CameraSystem/Element/Deleted"_hash: {
        //全部刷新用于清理失效元素
        for (auto& [name, pSolution] : this->solutions) {
            pSolution->Refresh();
        }
        break;
    }
    }
}

bool SolutionManager::HandleUpdate(CameraSystemIO* IO) {
    this->Update();
    if (this->CurrentSolution) {
        this->CurrentSolution->Refresh();//刷新当前调试的解决方案确保操作反馈及时（当前播放的解决方案由Playing_Call负责更新）
    }
    if (!this->Config.SolutionShortcutEnable)return this->Playing_Call(IO);
    //遍历
    for (const auto& [name, pSolution] : this->solutions) {
        //快捷键播放处理
        if (this->pInputSystem->CheckWithPack(pSolution->KCPack)) {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CameraSystem/Solution/Play"_hash);
            rp->str1 = pSolution.get()->name;
            this->PublishAsync(std::move(msg));
        }
        //后续其它任务待补充
    }
    return this->Playing_Call(IO);
}
//创建，得到，删除

bool SolutionManager::Solution_Create(const std::string& name) {
    // 检查是否已存在同名解决方案
    if (this->solutions.find(name) != this->solutions.end()) {
        this->LogError("解决方案名已占用！ 解决方案名：" + name);
        return false;
    }
    //输出成功信息
    this->LogSucc("成功创建解决方案！  解决方案名：" + name);
    //创建新解决方案
    std::unique_ptr<Solution> newSolution = std::make_unique<Solution>(name);
    this->solutions[name] = std::move(newSolution);

    return true;
}
bool SolutionManager::Solution_SaveAll() {
    if (this->solutions.empty()) {
        this->LogWarning("尝试在没有任何解决方案的情况下保存");
        return true;
    }
    std::filesystem::path SolutionFolderPath = this->Path()->PathGetFromKey("Solutions");
    //遍历所有解决方案保存
    for (const auto& [name, solution] : this->solutions) {
        if (!solution->dirty) {
            continue;//不脏不需保存
        }
        auto [ok, msg] = solution->Save(SolutionFolderPath);
        if (!ok) {
            this->LogError(std::move(msg));
            return false;
        }
        this->LogSucc(std::move(msg));
    }
    this->LogSucc("成功保存所有解决方案到文件！");
    return true;
}
bool SolutionManager::Solution_Load(const std::filesystem::path& FullPath) {
    // 输出调试信息
    this->LogInfo("尝试从yaml文件加载解决方案，文件路径：" + FullPath.string());
    // 检查文件本身存在性
    if (!std::filesystem::exists(FullPath)) {
        this->LogError("yaml文件不存在！文件路径：" + FullPath.string());
        return false;
    }
    try {
        YAML::Node root = YAML::LoadFile(FullPath.string());
        // 获取解决方案名称并检查是否为空
        std::string NewSolutionName = root["name"].as<std::string>();

        if (NewSolutionName.empty()) {
            this->LogError("尝试从yaml文件加载解决方案失败，解决方案名称为空！");
            return false;
        }

        // 检查是否存在同名解决方案
        if (this->solutions.find(NewSolutionName) != this->solutions.end()) {
            this->LogError("解决方案名已占用，无法从yaml文件加载解决方案！ 解决方案名：" + std::move(NewSolutionName));
            return false;
        }
        // 获取持续时长信息
        float TargetDurationTime = root["duration"].as<float>();
        // 制作解决方案
        auto newSolution = std::make_unique<Solution>(NewSolutionName);
        auto [ok, msg] = newSolution->Load(root, this->EManager);

        if (!ok) {
            this->LogError(std::move(msg));
            return false;
        }

        // 检验时间关系
        if (newSolution->totalDurationTime != TargetDurationTime) {
            this->LogWarning("该解决方案实际持续时长与预估持续时长不同，可能出现问题");
        }

        // 添加进解决方案组
        this->solutions[NewSolutionName] = std::move(newSolution);
        this->LogSucc(std::move(msg));
        this->LogLine();
        return true;
    }
    catch (const YAML::Exception& e) {
        this->LogError("在加载yaml时遇到异常：" + std::string(e.what()));
        return false;
    }
}

bool SolutionManager::Solution_Delete(const std::string& name) {
    //安全检查
    if (name.empty()) {
        this->LogError("尝试删除空名称的解决方案！");
        return false;
    }

    auto it = this->solutions.find(name);
    if (it == this->solutions.end()) {
        this->LogError("未找到指定名称的解决方案：" + name);
        return false;
    }

    //检查是否正在播放此解决方案
    if (this->Playing) {
        if (this->Playing_pSolution == it->second.get()) {
            this->Playing_Disable(); //禁用播放
            this->Playing_pSolution = nullptr;
        }
    }

    //检查是否当前正在操作此解决方案
    if (this->CurrentSolution) {
        if (this->CurrentSolution == it->second.get())
            this->CurrentSolution = nullptr;
    }

    this->solutions.erase(it);

    this->LogSucc("成功删除解决方案：" + name);
    return true;
}
bool SolutionManager::Solution_ClearAll() {
    //禁用播放
    this->Playing_Disable();
    this->Playing_pSolution = nullptr;
    //清空当前操作解决方案
    this->CurrentSolution = nullptr;

    if (this->solutions.empty()) {
        this->LogWarning("当前没有任何解决方案，跳过清空操作！");
        return true;
    }
    //清空所有解决方案
    this->solutions.clear();
    this->LogSucc("成功删除所有解决方案！");
    return true;
}

void SolutionManager::Playing_Solution(const std::string& name) {
    auto it = this->solutions.find(name);
    if (it == this->solutions.end()) {
        this->LogError(std::format("目标解决方案不存在：{}", name));
        return;
    }
    this->Playing_pSolution = it->second.get();

    switch (this->Playing_pSolution->playmode) {
    case PlaybackMode::Orchestration:
        this->Playing_pSolution->SetSolutionOffset(this->pTimeline->GetTime());//偏移时间轴播放
        this->LogInfo(std::format("偏移时间轴播放，偏移时间设置为：{}", this->pTimeline->GetTime()));
        break;
    case PlaybackMode::Activation:
        this->Playing_pSolution->SetSolutionOffset(0);
        break;
    }
    this->Playing = true;
    this->PublishAsync("CameraSystem/Play/Started"_hash);
    this->LogInfo(std::format("播放解决方案：{}", name));
    return;
}
void SolutionManager::Playing_Disable() {
    this->Playing = false;
    this->PublishAsync("CameraSystem/Play/Ended"_hash);
    this->LogInfo("已关闭播放");
}
bool SolutionManager::Playing_Call(CameraSystemIO* IO) {
    if (!this->Playing) {
        return false;
    }
    //调用插值
    //理论上如果超出解决方案工作时间区，就不再调用
    if (!this->Playing_pSolution) {
        this->Playing_Disable();
        return false;
    }

    IO->SolutionTime = this->pTimeline->GetTime();
    IO->FrameGameTime = this->pTimeline->GetTime();
    IO->isPlaying = this->Playing;

    if (!this->Playing_pSolution->Call(IO)) {
        // 这里不关闭播放，因为解决方案可能还有内容
        // 不应该由管理器因为仅仅没有结果就关闭
        if (IO->isPlaying == false) {
            // 如果解决方案自己关闭了播放
            this->Playing_Disable();
            return false;
        }
        return false;
    }
    if (this->Config.PlayingDraw) {
        auto frame = this->drawCamera.Write();
        *frame = IO->Frame;
        this->needDrawCamera.store(true, std::memory_order_release);
    }
    else {
        this->needDrawCamera.store(false, std::memory_order_release);
    }
    return this->Config.PlayingOverride;
}