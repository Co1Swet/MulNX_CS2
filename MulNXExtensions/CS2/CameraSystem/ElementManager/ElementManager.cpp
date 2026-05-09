#include "ElementManager.hpp"
#include <MulNX/MulNX.hpp>
#include <MulNXExtensions/CS2/TimeController/TimeController.hpp>
#include <MulNXExtensions/CS2/ViewController/ViewController.hpp>
#include <MulNXExtensions/CS2/CameraSystem/CameraSystem.hpp>
#include <MulNXExtensions/CS2/CameraSystem/CameraDrawer/CameraDrawer.hpp>
#include <MulNXExtensions/CS2/CameraSystem/SolutionManager/SolutionManager.hpp>
#include <MulNXExtensions/CS2/CameraSystem/ProjectManager/ProjectManager.hpp>

bool ElementManager::MenuElement(MulNX::UINode* node) {
    // 展示预览功能相关状态
    ImGui::TextUnformatted(I18n(
        "camsys.elem.preview_status",
        this->OnPreview ? I18n("text.opened") : I18n("text.closed"),
        this->Preview_CurrentElement ? this->Preview_CurrentElement->GetName() : I18n("text.none"),
        this->Preview_TimeSchema
    ).c_str());

    ImGui::Separator();

    // 元素总设置
    if (ImGui::CollapsingHeader(I18n("camsys.elem.settings").c_str())) {
        ImGui::Checkbox(I18n("camsys.elem.preview_draw").c_str(), &this->Config.PreviewDraw);
        ImGui::Checkbox(I18n("camsys.elem.preview_override").c_str(), &this->Config.PreviewOverride);
    }

    // 创建元素
    if (ImGui::CollapsingHeader(I18n("camsys.elem.create").c_str())) {
        ImGui::Text(I18n("camsys.elem.new_name").c_str());
        ImGui::SameLine();
        static std::string newElementName = "";
        ImGui::InputText("##新元素名", &newElementName);
        // 创建自由摄像机轨道
        if (ImGui::Button(I18n("camsys.elem.new_free_camera_path").c_str())) {
            if (newElementName.empty()) {
                this->ISys().LogError(I18n("result.error_empty_name").c_str());
            }
            else {
                auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Element/Create"_hash);
                rp->str1 = std::move(newElementName);
                this->ISys().PublishAsync(std::move(msg));
            }
            newElementName.clear();
        }
    }
    // 展示修改元素
    if (ImGui::CollapsingHeader(I18n("camsys.elem.list").c_str())) {
        for (const auto& [name, element] : this->elements) {
            this->Element_ShowInLine(element);
        }
    }

    return true;
}

void ElementManager::Element_ShowInLine(const std::shared_ptr<ElementBase> element) {
    ImGui::Text(I18n("camsys.elem.name_label").c_str());
    ImGui::SameLine();

    if (ImGui::Selectable(element->Name.data(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
        if (ImGui::IsMouseDoubleClicked(0)) {
            this->CurrentElement.store(element, std::memory_order_release);
            this->ShowWindow.store(true, std::memory_order_release);
        }
    }

    if (ImGui::BeginPopupContextItem((I18n("camsys.elem.context_menu") + element->Name).c_str())) {
        if (ImGui::MenuItem(I18n("text.copy_name").c_str())) {
            ImGui::SetClipboardText(element->Name.c_str());
        }
        if (element->Drawable) {
            MulNX::UI::Checkbox(I18n("camsys.elem.draw").c_str(), element->draw);
        }
        if (ImGui::MenuItem(I18n("text.delete").c_str())) {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Element/Delete"_hash);
            rp->str1 = std::move(element->Name);
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();
    ImGui::Text(I18n("camsys.elem.type_duration", element->TypeGet_String(), std::to_string(element->DurationTime)).c_str());
}

bool ElementManager::UINodeFunc(MulNX::UINode* node) {
    std::unique_lock lock(this->CamSys()->smutex);
    for (auto& [name, elem] : this->elements) {
        elem->DrawBase(this->CamDrawer, this->CS2View()->GetViewMatrix(), this->CS2View()->GetWinWidth(), this->CS2View()->GetWinHeight());
    }
    if (this->needDrawCamera.load(std::memory_order_acquire)) {
        auto frame = this->drawCamera.Read();
        this->CamDrawer->DrawFrameCamera(*frame, I18n("camsys.elem.preview_draw_label").c_str());
    }
    auto w = MulNX::UI::RAIIWindow("元素调试", this->ShowWindow);
    if (!w)return true;
    // 检查当前是否有操作元素
    auto current = this->CurrentElement.load(std::memory_order_acquire);
    if (current) {
        // 根据元素类型调用不同的调试菜单
        current->DebugUI(this);
    }
    // 如果没有操作元素
    else {
        ImGui::Text(I18n("text.no_selected").c_str());
    }
    return true;
}
//元素管理器基本函数
bool ElementManager::Init() {
    this->CamDrawer = &this->Core->ModuleManager()->FindModule<CameraSystem>("CameraSystem")->CamDrawer;
    this->SManager = this->Core->ModuleManager()->FindModule<SolutionManager>("SolutionManager");
    this->PManager = this->Core->ModuleManager()->FindModule<ProjectManager>("ProjectManager");

    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {return this->UINodeFunc(node);});
    this->SendUINode("MenuElement", [this](MulNX::UINode* node) {return this->MenuElement(node);});

    auto* PathManager = this->ISys().PathManager();
    if (PathManager->CreateKey("Elements", "Elements",
        [this](MulNX::PathManager* PathManager)->bool {
            auto Path = PathManager->PathGetFromKey("Elements");
            this->ISys().LogSucc("成功设置元素路径为：" + Path.string());
            return true;
        })) {
        PathManager->KeyBindDynamic("Elements", "CurrentProject");
    }

    this->ISys()
        .SubscribeAsync("Element/Create")
        .SubscribeAsync("Element/Delete");

    return true;
}

void ElementManager::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Element/Create"_hash: {
        auto& name = msg.asp.get<MulNX::NetExt>()->str1;
        std::unique_lock lock(this->CamSys()->smutex);
        if (!this->Element_Create(ElementType::FreeCameraPath, name)) {
            this->ISys().LogError(std::format("元素创建失败：{}", name));
        }
        break;
    }
    case "Element/Delete"_hash: {
        auto& name = msg.asp.get<MulNX::NetExt>()->str1;
        std::unique_lock lock(this->CamSys()->smutex);
        if (!this->Element_Delete(name)) {
            this->ISys().LogError(std::format("元素删除失败：{}", name));
        }
        break;
    }
    }
}

void ElementManager::HandleUpdate() {
    this->Update();
    if (this->OnPreview) {
        CameraSystemIO IO;
        IO.ElementTime = this->CS2Time()->GetReal();
        IO.FrameGameTime = this->CS2Time()->GetReal();
        if (this->Preview_Call(&IO)) {
            //自由摄像机轨道预览
            if (this->Preview_CurrentElement->Type == ElementType::FreeCameraPath) {
                if (this->Config.PreviewOverride) {
                    this->CS2View()->CameraSystemIOOverride(&IO);
                }
                if (this->Config.PreviewDraw) {
                    auto frame = this->drawCamera.Write();
                    *frame = IO.Frame;
                    this->needDrawCamera.store(true, std::memory_order_release);
                }
                else {
                    this->needDrawCamera.store(false, std::memory_order_release);
                }
            }
        }
        //其它类型预览
    }
}
//ElementBase，Create和Get已在头文件中实现

//创建元素函数，支持传递任意参数给元素构造函数
ElementBase* ElementManager::Element_Create(const ElementType type, const std::string& name) {
    // 检查是否已存在同名元素
    if (this->elements.find(name) != this->elements.end()) {
        this->ISys().LogError("元素名已占用！ 元素名：" + name);
        return nullptr;
    }
    std::shared_ptr<ElementBase> pElement = nullptr;
    // 分发到具体类型的加载函数
    switch (type) {
    case ElementType::FreeCameraPath:
        pElement = std::make_shared<FreeCameraPath>(name);
        break;
    case ElementType::ElementBase:
        break;
    case ElementType::None:
        break;
    }
    // 输出成功信息
    this->ISys().LogSucc("成功创建元素！  元素名：" + name);
    // 设置元素类型
    pElement->Type = type;
    // 添加进Elements
    this->elements[name] = std::move(pElement);
    return this->elements[name].get();
}

bool ElementManager::Element_SaveAll() {
    //检查是否有元素
    if (this->elements.empty()) {
        this->ISys().LogWarning("当前没有任何元素，跳过保存操作！");
        return true;
    }
    std::filesystem::path ElementFolderPath = this->ISys().PathManager()->PathGetFromKey("Elements");
    //遍历所有元素并保存
    for (const auto& [name, elem] : this->elements) {
        if (!elem->Dirty) {
            //如果不脏则跳过保存
            continue;
        }
        auto [ok, msg] = elem->Save(ElementFolderPath);
        if (ok) {
            this->ISys().LogSucc(std::move(msg));
        }
        else {
            this->ISys().LogError(std::move(msg));
            return false;
        }
    }
    this->ISys().LogSucc("成功保存所有元素到磁盘！");
    return true;
}
bool ElementManager::Element_Load(const std::filesystem::path& FullPath) {
    this->ISys().LogInfo("尝试从磁盘文件加载元素，文件路径：" + FullPath.string());
    // 检查文件本身存在性
    if (!std::filesystem::exists(FullPath)) {
        this->ISys().LogError("磁盘文件不存在！文件路径：" + FullPath.string());
        return false;
    }

    try {
        YAML::Node root = YAML::LoadFile(FullPath.string());

        // 获取元素类型
        std::string NewElementTypeString = root["type"].as<std::string>();
        ElementType NewElementType = ElementType_StringToEnum(NewElementTypeString);
        if (static_cast<int>(NewElementType) <= 0) {
            this->ISys().LogError("尝试从磁盘文件加载元素失败，不可加载的元素类型！");
            return false;
        }

        // 获取元素名称
        std::string NewElementName = root["name"].as<std::string>();
        // 检查元素名是否为空
        if (NewElementName.empty()) {
            this->ISys().LogError("尝试从磁盘文件加载元素失败，元素名称为空！");
            return false;
        }
        // 检查是否存在同名元素
        if (this->elements.find(NewElementName) != this->elements.end()) {
            this->ISys().LogError("元素名已占用，无法从磁盘文件加载元素！ 元素名：" + NewElementName);
            return false;
        }
        // 创建基类指针
        this->ISys().LogInfo("尝试进行分发，元素类型为 " + NewElementTypeString + " ，文件路径：" + FullPath.string());

        auto pElement = this->Element_Create(NewElementType, NewElementName);
        // 判空
        if (!pElement) {
            this->ISys().LogError("尝试从磁盘文件加载元素失败，无法创建指定类型的元素实例！ 元素类型：" + NewElementTypeString);
            return false;
        }
        // 统一加载信息
        auto [ok, msg] = pElement->Load(root);
        if (!ok) {
            this->ISys().LogError(std::move(msg));
            return false;
        }
        pElement->Refresh();
        pElement->Dirty = false;// 刚刚进入内存，非脏
        this->ISys().LogSucc(std::move(msg));
        return true;
    }
    catch (...) {
        MulNX::ErrorTerminate("元素加载异常");
    }
}
bool ElementManager::Element_Delete(const std::string Name) {
    // 安全检查
    if (Name.empty()) {
        this->ISys().LogError("尝试删除空名称的元素！");
        return false;
    }

    // 获取迭代器
    auto it = this->elements.find(Name);
    // 判空
    if (it == this->elements.end()) {
        this->ISys().LogError("未找到指定名称的元素：" + Name);
        return false;
    }

    // 检查是否正在预览此元素
    if (this->Preview_CurrentElement && this->Preview_CurrentElement->Name == Name) {
        this->Preview_Disable(); // 禁用预览
    }

    // 检查是否当前正在操作此元素
    auto current = this->CurrentElement.load(std::memory_order_acquire);
    if (current && current->Name == Name) {
        this->CurrentElement = nullptr;
    }

    // 先标记为需要清理
    it->second->NeedBeDelete = true;
    // 通过迭代器删除元素
    this->elements.erase(it);
    // 添加刷新信息
    this->ISys().PublishAsync("CameraSystem/Element/Deleted"_hash);

    this->ISys().LogSucc("成功删除元素：" + Name);
    return true;
}
bool ElementManager::Element_ClearAll() {
    // 检查是否有元素
    if (this->elements.empty()) {
        this->ISys().LogWarning("当前没有任何元素，跳过清空操作！");
        return true;
    }
    // 禁用预览
    this->Preview_Disable();
    this->Preview_CurrentElement = nullptr;
    // 清空当前操作元素
    this->CurrentElement = nullptr;
    // 把所有元素标记为需要清理并从Elements中释放
    for (auto& [name, elem] : this->elements) {
        elem->NeedBeDelete = true;
    }
    this->elements.clear();
    // 添加刷新信息
    this->ISys().PublishAsync("CameraSystem/Element/Deleted"_hash);
    this->ISys().LogSucc("成功清空所有元素！");
    return true;
}

//预览相关
void ElementManager::Preview_Enable() {
    if (!this->Preview_CurrentElement) {
        this->ISys().LogError("无法开启预览：未设置预览元素！");
        return;
    }
    this->OnPreview = true;
    this->ISys().PublishAsync("CameraSystem/Preview/Started"_hash);
    this->ISys().LogInfo("已开启预览");
}
void ElementManager::Preview_Disable() {
    this->OnPreview = false;
    this->CS2View()->ClearViewOverride();
    this->ISys().PublishAsync("CameraSystem/Preview/Ended"_hash);
    this->ISys().LogInfo("已关闭预览");
}
void ElementManager::Preview_SetElement(const std::string& name) {
    auto it = this->elements.find(name);
    if (it == this->elements.end()) {
        this->ISys().LogError("找不到目标元素   元素名：" + name);
        return;
    }
    this->Preview_CurrentElement = it->second;
    this->ISys().LogInfo("准备预览该元素   元素名：" + name);
}
void ElementManager::Preview_SetPreviewSchema(const float Time) {
    this->Preview_TimeSchema = Time;
    this->ISys().LogInfo("元素预览时间偏移设置为：" + std::to_string(this->Preview_TimeSchema));
    this->Preview_EndTime = this->Preview_CurrentElement->StartTime + this->Preview_CurrentElement->DurationTime;
}
bool ElementManager::Preview_Call(CameraSystemIO* IO) {
    if (!this->OnPreview)return false;
    if (!this->Preview_CurrentElement) {
        this->Preview_Disable();
        return false;
    }
    IO->ElementTime += this->Preview_CurrentElement->GetStartTime() - this->Preview_TimeSchema;
    if (!this->Preview_CurrentElement->CalculateFrame(IO)) {
        this->Preview_Disable();
        return false;
    }
    return true;
}