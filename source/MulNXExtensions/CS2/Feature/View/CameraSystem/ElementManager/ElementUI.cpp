#include "ElementManager.hpp"
#include <Support/TimeController/TimeController.hpp>

bool ElementManager::MenuElement()const {
    std::shared_lock lock(this->smutex);

    ImGui::SeparatorText("预览总控");
    ImGui::Text(std::format("是否允许预览摄像机绘制：{}", this->Config.PreviewDraw ? "允许" : "不允许").c_str());
    if (ImGui::Button("启用预览摄像机绘制")) {
        this->PublishAsync("Campath/Preview/Draw/Enable"_hash);
    }
    ImGui::SameLine();
    if (ImGui::Button("禁用预览摄像机绘制")) {
        this->PublishAsync("Campath/Preview/Draw/Disable"_hash);
    }
    ImGui::Text(std::format("是否允许预览摄像机覆盖游戏摄像机：{}",
        this->Config.PreviewOverride ? "允许" : "不允许").c_str());
    if (ImGui::Button("启用预览摄像机覆盖")) {
        this->PublishAsync("Campath/Preview/Override/Enable"_hash);
    }
    ImGui::SameLine();
    if (ImGui::Button("禁用预览摄像机覆盖")) {
        this->PublishAsync("Campath/Preview/Override/Disable"_hash);
    }

    ImGui::SeparatorText("运镜创建");
    static std::string newElementName = "";
    ImGui::InputText("新轨道名", &newElementName);
    // 创建自由摄像机轨道
    if (ImGui::Button("创建")) {
        if (newElementName.empty()) {
            this->LogError("运镜名不能为空！");
        }
        else {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Element/Create"_hash);
            rp->str1 = std::move(newElementName);
            this->PublishAsync(std::move(msg));
        }
        newElementName.clear();
    }

    ImGui::SeparatorText("运镜列表");
    for (const auto& [name, element] : this->elements) {
        this->Element_ShowInLine(element);
    }

    return true;
}

void ElementManager::Element_ShowInLine(const std::shared_ptr<const FreeCameraPath> element)const {
    ImGui::Text(I18n("camsys.elem.name_label").c_str());
    ImGui::SameLine();

    if (element->GetName().empty())return;

    if (ImGui::Selectable(element->GetName().c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
        if (ImGui::IsMouseDoubleClicked(0)) {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Campath/OpenDebug"_hash);
            rp->str1 = element->GetName();
            this->PublishAsync(std::move(msg));
        }
    }

    if (ImGui::BeginPopupContextItem((I18n("camsys.elem.context_menu") + element->GetName().c_str()).c_str())) {
        if (ImGui::MenuItem("复制运镜名称")) {
            ImGui::SetClipboardText(element->GetName().c_str());
        }
        if (ImGui::MenuItem("删除运镜")) {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Element/Delete"_hash);
            rp->str1 = element->GetName();
            this->PublishAsync(std::move(msg));
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();
    ImGui::Text(I18n("camsys.elem.type_duration", "自由摄像机轨道", std::to_string(element->DurationTime)).c_str());
}

void ElementManager::DebugUI(const FreeCameraPath* campath)const {
    ImGui::TextUnformatted(campath->GetBaseInfo().c_str());
    ImGui::SeparatorText("关键帧列表");

    static int indexForReset = -1;
    static int PreIndex = -2;

    for (size_t i = 0; i < campath->CameraKeyframes.size(); ++i) {
        const MulNX::Math::CameraKeyframe& keyframe = campath->CameraKeyframes.at(i);
        if (ImGui::Selectable(std::to_string(i).c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
            indexForReset = i;
            if (ImGui::IsMouseDoubleClicked(0)) {
                auto pos = keyframe.GetPosition();
                auto rot = keyframe.GetRotationEuler();
                auto dof = keyframe.GetDOF();
                this->CS2View->spec_goto_ex(pos, rot);
                this->CS2View->SetDOF(dof);
                if (this->pInputSystem->IsKeyPressed(VK_MENU)) {
                    this->CS2Time->JumpReal(keyframe.time);
                }
            }
        }
        ImGui::SameLine();
        ImGui::Text(I18n("free_campath.fmt", i, keyframe.GetMsg()).c_str());
    }

    ImGui::Text(std::format("当前轨道是否绘制：{}", campath->draw.load(std::memory_order_acquire) ? "绘制" : "不绘制").c_str());
    if (ImGui::Button("为该轨道开启绘制")) {
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Campath/DrawOne"_hash);
        rp->str1 = campath->GetName();
        this->PublishAsync(std::move(msg));
    }
    ImGui::SameLine();
    if (ImGui::Button("为该轨道关闭绘制")) {
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Campath/UnDrawOne"_hash);
        rp->str1 = campath->GetName();
        this->PublishAsync(std::move(msg));
    }

    static auto kAdd = this->Shortcut()->GetButton("place camera").value();
    if (ImGui::Button("捕获当前位置以添加关键帧（或按下Tab键）") || this->pInputSystem->CheckWithPack(kAdd)) {        
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Campath/AddKeyframe"_hash);
        auto&& [t, x, y, z, fov, rx, ry, rz, d1, d2, d3, d4] =
            msg.Access<float, float, float, float, float, float, float, float, float, float, float, float>();
        t = this->pTimeline->GetTime();
        auto view = this->CS2View->GetView();
        x = view.position.x;
        y = view.position.y;
        z = view.position.z;
        fov = view.FOV;
        rx = view.rotation.x;
        ry = view.rotation.y;
        rz = view.rotation.z;
        d1 = view.dof.NearBlurry;
        d2 = view.dof.NearCrisp;
        d3 = view.dof.FarCrisp;
        d4 = view.dof.FarBlurry;
        rp->str1 = campath->GetName();
        this->PublishAsync(std::move(msg));
    }
    if (ImGui::Button("预览运镜")) {
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Campath/Preview"_hash);
        auto&& [previewOffset] = msg.Access<float>();
        previewOffset = 0.0f;
        rp->str1 = campath->GetName();
        this->PublishAsync(std::move(msg));
    }
    if (ImGui::Button("复制运镜名称")) {
        ImGui::SetClipboardText(campath->GetName().c_str());
    }
    if (ImGui::Button("清空关键帧（或双击delete键）") || this->pInputSystem->CheckComboClick(VK_DELETE, 2)) {
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Campath/ClearOne"_hash);
        rp->str1 = campath->GetName();
        this->PublishAsync(std::move(msg));
    }
    if (ImGui::Button("删除运镜")) {
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Element/Delete"_hash);
        rp->str1 = std::move(campath->GetName());
        this->PublishAsync(std::move(msg));
    }

    ImGui::Separator();

    if (0 <= indexForReset && indexForReset < campath->CameraKeyframes.size()) {
        const auto& keyframe = campath->GetKeyFrame(indexForReset);
        ImGui::Text(I18n("free_campath.fmt_edit", indexForReset, keyframe.GetMsg()).c_str());
        ImGui::Separator();

        static float temptime{};
        static DirectX::XMFLOAT4 tempPositionAndFOV{};
        static DirectX::XMFLOAT3 tempRotationEuler{};
        if (indexForReset != PreIndex) {
            temptime = keyframe.time;
            tempPositionAndFOV = keyframe.GetPositionAndFOV();
            tempRotationEuler = keyframe.GetRotationEuler();
        }

        ImGui::SliderFloat(I18n("math.time").c_str(), &temptime, 0, 20000);

        ImGui::SliderFloat3(I18n("math.pos").c_str(), &tempPositionAndFOV.x, -2000.0, 2000, 0);
        ImGui::SliderFloat(I18n("math.yaw").c_str(), &tempRotationEuler.x, -89.0, 89.0);
        ImGui::SliderFloat(I18n("math.pitch").c_str(), &tempRotationEuler.y, -179.0, 179.0);
        ImGui::SliderFloat(I18n("math.roll").c_str(), &tempRotationEuler.z, -179.0, 179.0);
        ImGui::SliderFloat(I18n("math.fov").c_str(), &tempPositionAndFOV.w, 10, 170);

        this->CamDrawer->DrawCamera(DirectX::XMFLOAT3{ tempPositionAndFOV.x,tempPositionAndFOV.y ,tempPositionAndFOV.z }, tempRotationEuler, "目标摄像机关键帧");
        if (ImGui::Button("修改选中的关键帧")) {
            auto [del, r] = MulNX::Message::Create<MulNX::NetExt>("Campath/DeleteKeyframe"_hash);
            r->str1 = campath->GetName();
            auto&& [index] = del.Access<size_t>();
            index = indexForReset;
            this->PublishAsync(std::move(del));

            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Campath/AddKeyframe"_hash);
            auto&& [t, x, y, z, fov, rx, ry, rz, d1, d2, d3, d4] =
                msg.Access<float, float, float, float, float, float, float, float, float, float, float, float>();
            t = temptime;
            x = tempPositionAndFOV.x;
            y = tempPositionAndFOV.y;
            z = tempPositionAndFOV.z;
            fov = tempPositionAndFOV.w;
            rx = tempRotationEuler.x;
            ry = tempRotationEuler.y;
            rz = tempRotationEuler.z;
            d1 = -1000.0f;
            d2 = 0.0f;
            d3 = 1000.0f;
            d4 = 3000.0f;
            rp->str1 = campath->GetName();
            this->PublishAsync(std::move(msg));
            PreIndex = -1;
        }
        if (ImGui::Button("删除选中的关键帧")) {
            auto [del, r] = MulNX::Message::Create<MulNX::NetExt>("Campath/DeleteKeyframe"_hash);
            r->str1 = campath->GetName();
            auto&& [index] = del.Access<size_t>();
            index = indexForReset;
            this->PublishAsync(std::move(del));
            PreIndex = -1;
        }
        if (ImGui::Button("复制选中的关键帧")) {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Campath/CopyKeyframe"_hash);
            rp->str1 = campath->GetName();
            auto&& [index] = msg.Access<size_t>();
            index = indexForReset;
            this->PublishAsync(std::move(msg));
            PreIndex = -1;
        }
    }
    PreIndex = indexForReset;
}
void ElementManager::UINodeFunc()const {
    std::shared_lock lock(this->smutex);
    for (auto& [name, elem] : this->elements) {
        elem->Draw(this->CamDrawer, this->CS2View->GetViewMatrix(), this->CS2View->GetWinWidth(), this->CS2View->GetWinHeight());
    }
    if (this->needDrawCamera.load(std::memory_order_acquire) && this->Config.PreviewDraw) {
        auto frame = this->drawCamera.Read();
        this->CamDrawer->DrawFrameCamera(*frame, I18n("camsys.elem.preview_draw_label").c_str());
    }
    auto w = MulNX::UI::RAIIWindow("元素调试", this->showWindow);
    if (!w || !w.ShouldDraw())return;
    auto current = this->CurrentElement.load(std::memory_order_acquire);
    if (current) {
        this->DebugUI(current.get());
    }
    else {
        ImGui::Text("当前没有选择任何运镜");
    }
}