#include "FreeCameraPath.hpp"

#include <MulNXExtensions/CS2/CSController/CSController.hpp>
#include <MulNXExtensions/CS2/CameraSystem/CameraDrawer/CameraDrawer.hpp>
#include <MulNXExtensions/CS2/CameraSystem/ElementManager/ElementManager.hpp>

std::string FreeCameraPath::GetPrivateMsg()const {
    std::ostringstream oss;
    for (size_t i = 0; i < this->CameraKeyframes.size(); ++i) {
        const MulNX::Math::CameraKeyframe& keyframe = this->CameraKeyframes.at(i);
        oss << I18n("free_campath.fmt", i, keyframe.GetMsg());
    }
    return oss.str();
}

void FreeCameraPath::DebugUI(ElementManager* EManager) {
    ImGui::TextUnformatted(this->GetBaseInfo().c_str());

    static int IndexForReset = -1;
    static int PreIndex = -2;

    for (size_t i = 0; i < this->CameraKeyframes.size(); ++i) {
        const MulNX::Math::CameraKeyframe& keyframe = this->CameraKeyframes.at(i);
        if (ImGui::Selectable(std::to_string(i).c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
            IndexForReset = i;
            if (ImGui::IsMouseDoubleClicked(0)) {
                auto pos = keyframe.GetPosition();
                auto rot = keyframe.GetRotationEuler();
                auto dof = keyframe.GetDOF();
                auto* pCS2 = EManager->CS2();
                pCS2->spec_goto_ex(pos, rot);
                pCS2->SetDOF(dof);
                if (pCS2->pInputSystem->IsKeyPressed(VK_MENU)) {
                    pCS2->Time()->JumpReal(keyframe.time);
                }
            }
        }
        ImGui::SameLine();
        ImGui::Text(I18n("free_campath.fmt", i, keyframe.GetMsg()).c_str());
    }

    if (ImGui::Button(I18n("free_campath.add").c_str()) || EManager->pInputSystem->CheckComboClick('F', 1)) {
        MulNX::Math::CameraKeyframe keyframe;
        keyframe.time = EManager->CS2()->Time()->GetReal();
        auto view = EManager->CS2()->GetView();
        keyframe.PositionAndFOV = view.ToPositionAndFOV();
        keyframe.RotationQuat = view.ToRotationQuat();
        keyframe.dof = view.ToDOFPack();
        this->AddKeyframe(keyframe);
    }

    if (ImGui::Button(I18n("text.clear").c_str()) || EManager->pInputSystem->CheckComboClick(VK_DELETE, 2)) {
        this->Clear();
    }

    if (ImGui::Button(I18n("text.normalize").c_str())) {
        this->TimeNormalize();
    }

    if (ImGui::Button(I18n("text.preview").c_str())) {
        EManager->Preview_SetElement(this->Name);
        EManager->Preview_SetPreviewSchema(EManager->CS2()->Time()->GetReal());
        EManager->Preview_Enable();
    }

    ImGui::Separator();

    if (0 <= IndexForReset && IndexForReset < this->CameraKeyframes.size()) {
        const MulNX::Math::CameraKeyframe& keyframe = this->GetKeyFrame(IndexForReset);
        ImGui::Text(I18n("free_campath.fmt_edit", IndexForReset, keyframe.GetMsg()).c_str());
        ImGui::Separator();

        static float temptime{};
        static DirectX::XMFLOAT4 tempPositionAndFOV{};
        static DirectX::XMFLOAT3 tempRotationEuler{};
        if (IndexForReset != PreIndex) {
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

        EManager->CamSys()->CamDrawer.DrawCamera(DirectX::XMFLOAT3{ tempPositionAndFOV.x,tempPositionAndFOV.y ,tempPositionAndFOV.z }, tempRotationEuler, "目标摄像机关键帧");
        if (ImGui::Button(I18n("text.confirm_modify").c_str())) {
            // 构造临时摄像机关键帧
            MulNX::Math::CameraKeyframe tempKey;
            // 注入时间
            tempKey.time = temptime;
            // 注入位置和FOV
            tempKey.PositionAndFOV = DirectX::XMLoadFloat4(&tempPositionAndFOV);
            // 转换角度并注入
            DirectX::XMFLOAT4 tempRotationQuat;
            MulNX::Math::CSEulerToQuat(tempRotationEuler, tempRotationQuat);
            tempKey.RotationQuat = DirectX::XMLoadFloat4(&tempRotationQuat);
            // 擦除旧关键帧
            this->CameraKeyframes.erase(this->CameraKeyframes.begin() + IndexForReset);
            // 添加新关键帧
            this->AddKeyframe(std::move(tempKey));
            PreIndex = -1;
        }
        if (ImGui::Button(I18n("text.delete").c_str())) {
            //删除并刷新
            this->CameraKeyframes.erase(this->CameraKeyframes.begin() + IndexForReset);
            this->Refresh();
            PreIndex = -1;
        }
        if (ImGui::Button(I18n("text.copy").c_str())) {
            //拷贝复制
            this->AddKeyframe(this->GetKeyFrame(IndexForReset));
            PreIndex = -1;
        }
    }
    PreIndex = IndexForReset;
}

void FreeCameraPath::AddKeyframe(const MulNX::Math::CameraKeyframe& keyframe) {
	//按照时间排序插入
    auto it = std::lower_bound(this->CameraKeyframes.begin(), this->CameraKeyframes.end(), keyframe,
        [&](const MulNX::Math::CameraKeyframe& a, const MulNX::Math::CameraKeyframe& b) {//引用捕获加速
            return a.time < b.time;
        });
    this->CameraKeyframes.insert(it, keyframe); // 拷贝插入
    this->Refresh();

    return;
}

void FreeCameraPath::Refresh() {
    //标记为脏
    this->Dirty = true;
    if (this->CameraKeyframes.size() == 0) {
        this->StartTime = 0;
        this->EndTime = 0;
        this->DurationTime = 0;
        return;
    }
    else {
        this->StartTime = this->CameraKeyframes.front().time;
        this->EndTime = this->CameraKeyframes.back().time;
        this->DurationTime = this->EndTime - this->StartTime;
        return;
    }
    
    return;
}
void FreeCameraPath::TimeNormalize() {
    if (this->CameraKeyframes.front().time == 0)return;
    size_t Size = this->CameraKeyframes.size();
    if (!Size)return;
    if (Size == 1) {
        this->CameraKeyframes.at(0).time = 0;
        return;
    }
    float Schema = this->CameraKeyframes.front().time;
    for (int i = 1; i < Size; ++i) {
        this->CameraKeyframes.at(i).time -= Schema;
    }
    this->CameraKeyframes.front().time = 0;
    this->Refresh();
    return;
}
bool FreeCameraPath::Call(CameraSystemIO* IO)const {
    // 如果不在理论影响范围内，应当直接返回而不做任何修改
    // 是与被遍历的其它call一起工作

    // 处理空关键帧的情况
    if (this->CameraKeyframes.empty())return false;

    float Time = IO->ElementTime;

    // 查找当前时间所在的关键帧区间（使用绝对时间来搜索以匹配以绝对时间存储的关键帧）
    auto it = std::lower_bound(this->CameraKeyframes.begin(), this->CameraKeyframes.end(), Time,
        [](const MulNX::Math::CameraKeyframe& k, float t) {
            return k.time < t;
        });

    // 确保迭代器有效并安全地计算索引
    ptrdiff_t dist = std::distance(this->CameraKeyframes.begin(), it);
    size_t index = (dist == 0) ? 0 : static_cast<size_t>(dist - 1);

    // 保护：如果 index 位于最后一个元素，则没有下一个关键帧可用于插值
    if (index + 1 >= this->CameraKeyframes.size()) return false;

    // 获取相邻的四个关键帧用于插值
    const auto& k1 = this->CameraKeyframes[index];
    const auto& k2 = this->CameraKeyframes[index + 1];
    const auto& k0 = (index > 0) ? this->CameraKeyframes[index - 1] : k1;
    const auto& k3 = (index + 2 < this->CameraKeyframes.size()) ? this->CameraKeyframes[index + 2] : k2;

    // 计算当前片段的时间比例 (0~1)
    float segmentDuration = k2.time - k1.time;
    if (segmentDuration <= 0.0f) return false; // 避免除以零或无效的段

    float segmentTime = (Time - k1.time) / segmentDuration;

    // 位置和FOV插值 (Catmull-Rom)
    auto PositionAndFOV = DirectX::XMVectorCatmullRom(
        k0.PositionAndFOV,
        k1.PositionAndFOV,
        k2.PositionAndFOV,
        k3.PositionAndFOV,
        segmentTime);

    // 写入数据
    IO->Frame.view.position = { PositionAndFOV.m128_f32[0],PositionAndFOV.m128_f32[1],PositionAndFOV.m128_f32[2] };
    IO->Frame.view.FOV = PositionAndFOV.m128_f32[3];

    // 景深插值
    auto Dof = DirectX::XMVectorCatmullRom(
        k0.dof,
        k1.dof,
        k2.dof,
        k3.dof,
        segmentTime
    );

    IO->Frame.view.dof.NearBlurry = Dof.m128_f32[0];
    IO->Frame.view.dof.NearCrisp = Dof.m128_f32[1];
    IO->Frame.view.dof.FarCrisp = Dof.m128_f32[2];
    IO->Frame.view.dof.FarBlurry = Dof.m128_f32[3];

    // 旋转插值 (使用Squad提供高阶连续性)

    // 使用DirectXMath内置函数计算Squad控制点
    DirectX::XMVECTOR s1, s2, s3;
    DirectX::XMQuaternionSquadSetup(&s1, &s2, &s3,
        k0.RotationQuat,
        k1.RotationQuat,
        k2.RotationQuat,
        k3.RotationQuat);

    // 使用Squad进行插值
    DirectX::XMVECTOR rotation = DirectX::XMQuaternionSquad(k1.RotationQuat, s1, s2, s3, segmentTime);
    auto RotationQuat = DirectX::XMQuaternionNormalize(rotation);
    DirectX::XMFLOAT4 quat;
    DirectX::XMStoreFloat4(&quat, RotationQuat);
    MulNX::Math::CSQuatToEuler(quat, IO->Frame.view.rotation);
    
    IO->Frame.TargetOBMode = 4;

    return true;
}
//绘制函数（虚），各个元素按需实现
bool FreeCameraPath::Draw(CameraDrawer* CamDrawer, const float* Matrix, const float WinWidth, const float WinHeight)const {
    const auto& keyframes = this->GetAllKeyFrames();

    // 存储上一个关键帧的位置（用于连线）
    DirectX::XMFLOAT3 prevPosition{};
    bool firstFrame = true;

    // 遍历当前路径的所有关键帧
    for (int i = 0; i < keyframes.size(); ++i) {
        if (!Matrix)return false;
        const auto& keyframe = keyframes.at(i);
        // 绘制关键帧的摄像机
        std::string label = std::format("{} # {}", this->Name, i);
        CamDrawer->DrawCamera(keyframe.GetPosition(), keyframe.GetRotationEuler(), label.c_str());

        // 获取ImDrawList用于绘制连线
        ImDrawList* drawList = ImGui::GetBackgroundDrawList();

        // 如果不是第一个关键帧，绘制连线
        if (!firstFrame) {
            // 将3D位置转换为屏幕坐标
            DirectX::XMFLOAT2 prevScreenPos, currentScreenPos;

            // 使用CameraDrawer中的转换方法
            MulNX::Math::WorldToScreen(prevPosition, prevScreenPos, Matrix, WinWidth, WinHeight);

            MulNX::Math::WorldToScreen(keyframe.GetPosition(), currentScreenPos, Matrix, WinWidth, WinHeight);

            // 绘制连线
            drawList->AddLine(
                ImVec2(prevScreenPos.x, prevScreenPos.y),
                ImVec2(currentScreenPos.x, currentScreenPos.y),
                IM_COL32(0, 255, 255, 255), // 青色连线
                2.0f // 线宽
            );
        }
        // 保存当前位置用于下一次连线
        prevPosition = keyframe.GetPosition();
        firstFrame = false;
    }
    return true;
}

size_t FreeCameraPath::GetKeyFrameCount() const {
    return this->CameraKeyframes.size();
}
const MulNX::Math::CameraKeyframe& FreeCameraPath::GetKeyFrame(const size_t& index)const {
    return this->CameraKeyframes[index];
}
const std::vector<MulNX::Math::CameraKeyframe>& FreeCameraPath::GetAllKeyFrames()const {
    return this->CameraKeyframes;
}
void FreeCameraPath::Clear() {
    this->CameraKeyframes.clear();
    this->Refresh();
    return;
}
std::pair<bool, std::string> FreeCameraPath::SaveImpl(YAML::Node& root) {
    try {
        // 添加关键帧
        for (const auto& keyframe : this->CameraKeyframes) {
            // 创建关键帧节点
            YAML::Node keyframeNode;
            // 时间
            keyframeNode["T"] = keyframe.time;

            // 添加坐标节点
            DirectX::XMFLOAT4 posFov = keyframe.GetPositionAndFOV();
            YAML::Node posNode;
            posNode.push_back(posFov.x);
            posNode.push_back(posFov.y);
            posNode.push_back(posFov.z);
            posNode.SetStyle(YAML::EmitterStyle::Flow);
            keyframeNode["P"] = posNode;

            // 添加旋转节点
            DirectX::XMFLOAT3 euler = keyframe.GetRotationEuler();
            YAML::Node rotNode;
            rotNode.push_back(euler.x); // Pitch
            rotNode.push_back(euler.y); // Yaw
            rotNode.push_back(euler.z); // Roll
            rotNode.SetStyle(YAML::EmitterStyle::Flow);
            keyframeNode["R"] = rotNode;

            // 添加fov节点
            keyframeNode["F"] = posFov.w;

            // 添加景深节点
            auto dofs = keyframe.GetDOF();
            YAML::Node dofsNode;
            dofsNode.push_back(dofs.NearBlurry);
            dofsNode.push_back(dofs.NearCrisp);
            dofsNode.push_back(dofs.FarCrisp);
            dofsNode.push_back(dofs.FarBlurry);
            dofsNode.SetStyle(YAML::EmitterStyle::Flow);
            keyframeNode["D"] = dofsNode;

            // 设置关键帧节点为流样式
            keyframeNode.SetStyle(YAML::EmitterStyle::Flow);

            // 将关键帧节点挂载上去
            root["keyframes"].push_back(keyframeNode);
        }
        return { true, {} };
    }
    catch (const std::exception& e) {
        return { false, "保存YAML文件时发生错误：" + std::string(e.what()) };
    }
}

std::pair<bool, std::string> FreeCameraPath::Load(YAML::Node& root) {
    try {
        if (!root.IsMap()) return { false, "YAML文件根节点不是映射类型"};

        // 清空现有数据
        this->Clear();

        // 读取关键帧
        if (!root["keyframes"] || !root["keyframes"].IsSequence()) return { false, "找不到keyframes序列！" };

        for (const auto& keyframeNode : root["keyframes"]) {
            if (!keyframeNode.IsMap()) continue;

            MulNX::Math::CameraKeyframe keyframe;

            // 读取时间
            keyframe.time = keyframeNode["T"].as<float>();

            // 读取位置和FOV
            DirectX::XMFLOAT4 posFov;
            posFov.x = keyframeNode["P"][0].as<float>();
            posFov.y = keyframeNode["P"][1].as<float>();
            posFov.z = keyframeNode["P"][2].as<float>();
            posFov.w = keyframeNode["F"].as<float>();
            keyframe.PositionAndFOV = DirectX::XMLoadFloat4(&posFov);

            // 读取旋转（欧拉角）
            DirectX::XMFLOAT3 euler;
            euler.x = keyframeNode["R"][0].as<float>(); // Pitch
            euler.y = keyframeNode["R"][1].as<float>(); // Yaw
            euler.z = keyframeNode["R"][2].as<float>(); // Roll
            DirectX::XMFLOAT4 quat;
            MulNX::Math::CSEulerToQuat(euler, quat);
            keyframe.RotationQuat = DirectX::XMLoadFloat4(&quat);

            // 读取景深
            MulNX::Math::DOFParam dof;
            try {
                dof.NearBlurry = keyframeNode["D"][0].as<float>();
                dof.NearCrisp = keyframeNode["D"][1].as<float>();
                dof.FarCrisp = keyframeNode["D"][2].as<float>();
                dof.FarBlurry = keyframeNode["D"][3].as<float>();
            }
            catch (const std::exception&) {
                // 如果没有景深信息，使用默认值
                dof = { 0, 0, 10000, 10000 };
            }


            keyframe.dof = DirectX::XMVectorSet(
                dof.NearBlurry,
                dof.NearCrisp,
                dof.FarCrisp,
                dof.FarBlurry
            );

            this->AddKeyframe(std::move(keyframe));
        }

        return { true, "成功从YAML文件加载自由摄像机轨道信息！ 自由摄像机轨道 名：" + this->Name };
    }
    catch (const YAML::Exception& e) {
        return { false, "YAML解析错误：" + std::string(e.what()) };
    }
    catch (const std::exception& e) {
        return { false, "读取YAML文件时发生错误：" + std::string(e.what()) };
    }
}