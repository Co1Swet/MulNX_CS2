#include "CamPackManager.hpp"
#include <MulNX/Base/UI/UI.hpp>

void CamPackManager::Menu() const {
    std::shared_lock lock(this->smutex);

    ImGui::SeparatorText("运镜包总控");
    {
        bool enable = this->shortcutEnable.load(std::memory_order_acquire);
        if (ImGui::Checkbox("启用运镜包切换快捷键", &enable)) {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CamPack/ToggleShortcut"_hash);
            auto&& [v] = msg.Access<bool>();
            v = enable;
            this->PublishAsync(std::move(msg));
        }
    }

    ImGui::SeparatorText("运镜包创建");
    static std::string newName;
    ImGui::InputText("新运镜包名", &newName);
    if (ImGui::Button("创建运镜包")) {
        if (!newName.empty()) {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CamPack/Create"_hash);
            rp->str1 = newName;
            this->PublishAsync(std::move(msg));
            newName.clear();
        }
    }

    ImGui::SeparatorText("运镜包列表");
    for (const auto& [name, camPack] : this->camPacks) {
        const bool isSelected = (this->pOperatingCamPack == camPack);

        ImGui::PushID(name.c_str());
        if (ImGui::Selectable(name.c_str(), isSelected)) {
            auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CamPack/OpenDebug"_hash);
            rp->str1 = name;
            this->PublishAsync(std::move(msg));
        }
        ImGui::PopID();
    }
}
void CamPackManager::UI() const {
    std::shared_lock lock(this->smutex);

    if (!this->showWindow.load(std::memory_order_acquire)) return;

    this->CamPackDebugWindow();

    if (!this->bKCPWindow) return;

    auto buffer = this->bufKCPack.load();
    auto p = buffer.DebugWindow("运镜包快捷切换按键绑定", this->bKCPWindow);
    if (!p.first.has_value()) return;

    this->bufKCPack = *p.first;

    if (!p.second) return;

    this->bKCPWindow.store(false, std::memory_order_release);
    if (this->pOperatingCamPack) {
        this->pOperatingCamPack->SetKeyCheckPack(*p.first);
    }
}
void CamPackManager::CamPackDebugWindow() const {
    auto w = MulNX::UI::RAIIWindow("运镜包调试", this->showWindow);
    if (!w || !w.ShouldDraw()) return;

    if (!this->pOperatingCamPack) {
        ImGui::TextUnformatted("当前未选择任何运镜包");
        return;
    }

    const std::string packName = this->pOperatingCamPack->GetName();

    ImGui::TextUnformatted("调试中的运镜包：");
    ImGui::SameLine();
    ImGui::TextUnformatted(packName.c_str());

    if (ImGui::Button("切换到当前运镜包")) {
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CamPack/Apply"_hash);
        rp->str1 = packName;
        this->PublishAsync(std::move(msg));
    }
    ImGui::SameLine();
    if (ImGui::Button("删除当前运镜包")) {
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CamPack/Delete"_hash);
        rp->str1 = packName;
        this->PublishAsync(std::move(msg));
    }
    ImGui::SameLine();
    if (ImGui::Button("调试按键绑定")) {
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("CamPack/OpenKCPWindow"_hash);
        this->PublishAsync(std::move(msg));
    }
}