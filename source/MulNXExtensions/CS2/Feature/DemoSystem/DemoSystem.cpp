#include "DemoSystem.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <Support/TimeController/TimeController.hpp>
#include <shellapi.h>

void OpenFolder(const std::wstring& wpath) {
    ::ShellExecuteW(
        nullptr,          // 父窗口 HWND，可填你窗口句柄
        L"open",          // 动词：open / explore 都行
        wpath.c_str(),    // 文件夹路径
        nullptr,          // 参数（文件夹不需要）
        nullptr,          // 工作目录（默认）
        SW_SHOWNORMAL     // 显示方式
    );
}

void DemoSystem::Window(MulNX::UICoordinator* uico) {
    auto w = MulNX::UI::RAIIWindow("Demo系统", this->showWindow);
    if (!w) return;
    uico->CallbackCall("UI.Demos"_hash, nullptr);
    if (!w.ShouldDraw())return;
    uico->CallbackCall("UI.Demo.Main"_hash, nullptr);
    if (ImGui::Button("测试")) {
        OpenFolder(this->CS2Paths->demo.wstring());
    }
}

bool DemoSystem::Init() {
    (*this)
        .SubscribeAsync("Demo/Play")
        ;

    this->UIRegisterCallback("UI.Advanced", [this](auto&&...) {
        MulNX::UI::Checkbox("Demo系统", this->showWindow);
        });

    this->SendUIRoot(this->GetName(), [this](auto uico, auto&&...) {
        return this->Window(uico);
        });

    this->SendTask("Update", "DemoSys", [this]() {
        this->Update();
        return true;
        });

    this->PublishAsync("Demo/Refresh"_hash);

    return true;
}

void DemoSystem::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Demo/Play"_hash: {
        auto& path = msg.asp.get<MulNX::NetExt>()->str1;
        this->AsyncCommand(std::format("playdemo \"{}\"", path));
        break;
    }
    }
}