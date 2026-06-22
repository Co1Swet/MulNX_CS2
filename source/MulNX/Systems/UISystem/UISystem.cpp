#include "UISystem.hpp"
#include <MulNX/Base/CharUtility/CharUtility.hpp>
#include <MulNX/Base/UI/UI.hpp>
#include <MulNX/Systems/Systems.hpp>
#include <yaml-cpp/yaml.h>
#include <MulNXThirdParty/ImGuiStyleSerializer.h>
#include <Windows.h>
#include <fstream>

bool MulNX::UISystem::Menu(MulNX::UINode* node) {
    ImGui::Text(I18n("ui.style.info").c_str());
    if (ImGui::Button(I18n("ui.style.save").c_str())) {
        this->PublishAsync("UISystem/SaveStyle"_hash);
    }
    ImGui::Separator();
    ImGui::ShowStyleEditor();
    return true;
}

bool MulNX::UISystem::Init() {
    this->pCoordinator = this->FindModule<UICoordinator>("UICoordinator");

    ImGui::CreateContext();
    // 设置ini文件路径
    ImGuiIO& io = ImGui::GetIO();
    auto IniPath = this->PathGet("Config") / "MulNXUIConfig.ini";
    // 这里需要进行转换，以适配ImGui的接口
    this->strImguiIniPath = MulNX::CharUtility::FilePathToString(IniPath);
    io.IniFilename = this->strImguiIniPath.c_str();
    this->LoadFont();
    this->LoadStyle();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    (*this)
        .SubscribeAsync("UISystem/Start")
        .SubscribeAsync("UISystem/Toggle")
        .SubscribeAsync("UISystem/SaveStyle");

    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {return this->Menu(node);});

    return true;
}

void MulNX::UISystem::ProcessMsg(MulNX::Message& Msg) {
    switch (Msg.type) {
    case "UISystem/Start"_hash: {
        this->runFlag1.store(true);
        this->runFlag2.store(true);
        this->LogWarning("接收到启动消息，UI系统开始启动");

        break;
    }
    case "UISystem/Toggle"_hash: {
        this->runFlag2.store(!this->runFlag2.load());
        break;
    }
    case "UISystem/SaveStyle"_hash: {
        this->SaveStyle();
        break;
    }
    }
}

// ImGui窗口处理函数导入
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void MulNX::UISystem::HandleUpdate() {
    this->pCoordinator->HandleUpdate();
    this->Update();

    MulNX::Win32::Msg4 msg4;
    while (this->winMsgs.try_dequeue(msg4)) {
        ImGui_ImplWin32_WndProcHandler(msg4.hWnd, msg4.uMsg, msg4.wParam, msg4.lParam);
    }
}
int MulNX::UISystem::Render() {
    if (!this->runFlag2.load()) {
        this->WantCaptureMouse.store(false, std::memory_order_release);
        this->WantTextInput.store(false, std::memory_order_release);
        return 0;
    }
    ImGuiIO& io = ImGui::GetIO();
    if (this->WantCaptureMouse.load(std::memory_order_acquire)!= io.WantCaptureMouse) {
        this->WantCaptureMouse.store(io.WantCaptureMouse, std::memory_order_release);
    }
    if (this->WantTextInput.load(std::memory_order_acquire) != io.WantTextInput) {
        this->WantTextInput.store(io.WantTextInput, std::memory_order_release);
    }

    if (!this->FrameBefore())return 0;
    this->pCoordinator->Render();
    this->FrameBehind();

    return 0;
}

void MulNX::UISystem::LoadFont() {
    try {
        auto cfgPath = this->PathGet("Config") / "ui.yaml";
        this->LogInfo(I18n("ui.font.cfg.load", cfgPath.string()));
        YAML::Node root = YAML::LoadFile(cfgPath.string());
        auto fontFilePath = root["font"]["path"].as<std::string>();
        auto fontSize = root["font"]["size"].as<float>();
        this->LogSucc(I18n("ui.font.cfg.load_succ", fontFilePath, fontSize));
        this->LogInfo(I18n("ui.font.load", fontFilePath));

        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->AddFontFromFileTTF(fontFilePath.c_str(), fontSize);
        this->LogSucc(I18n("ui.font.load_succ", fontFilePath));
    }
    catch (const std::exception& e) {
        this->LogError(e.what());
    }
    catch (...) {
        MulNX::ErrorTerminate("Unknown Error On Load Font!");
    }
}
void MulNX::UISystem::LoadStyle() {
    try {
        // 加载Style
        auto stylePath = this->Path()->PathGetForShared("Config") / "ImStyle.yaml";
        this->LogInfo(I18n("ui.style.load", stylePath.string()));
        YAML::Node root = YAML::LoadFile(stylePath.string());
        ImGuiStyle newStyle;
        if (!ImGuiYaml::YamlToStyle(root, newStyle)) {
            this->LogError(I18n("ui.style.load_file_error", stylePath.string()));
            return;
        }
        ImGui::GetStyle() = newStyle;
        this->LogSucc(I18n("ui.style.load_succ", stylePath.string()));
    }
    catch (const std::exception& e) {
        this->LogError(e.what());
    }
    catch (...) {
        MulNX::ErrorTerminate("Unknown Error On Load Style!");
    }
}
void MulNX::UISystem::SaveStyle() {
    try {
        // 保存Style
        auto stylePath = this->Path()->PathGetForShared("Config") / "ImStyle.yaml";
        ImGuiStyle& style = ImGui::GetStyle();
        YAML::Node root;
        ImGuiYaml::StyleToYaml(style, root);
        std::ofstream fout(stylePath);
        fout << root;
        this->LogSucc(I18n("ui.style.save_succ", stylePath.string()));
    }
    catch (const std::exception& e) {
        this->LogError(I18n("ui.style.save_error_with", e.what()));
    }
    catch (...) {
        this->LogError(I18n("ui.style.save_error"));
    }
}