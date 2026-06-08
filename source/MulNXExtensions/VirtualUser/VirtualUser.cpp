#include "VirtualUser.hpp"
#include <MulNX/MulNX.hpp>
#include <MulNX/Base/UI/UI.hpp>
#include <MulNXExtensions/CS2/CSController/CSController.hpp>

bool VirtualUser::Init() {
    this->ISys()
#ifdef _DEBUG
        .SubscribeAsync("Core/Tick30min")
#endif // _DEBUG
        .SubscribeAsync("Core/Tick60")
        .SubscribeAsync("Command/SpecPlayer")
        .SubscribeAsync("Game/NewRound");

    this->ISys().SendUINode(this->GetName(), [this](MulNX::UINode* node)->bool {
        MulNX::UI::Checkbox("启用自动化增强（Alt+O可快速切换）", this->Enabled);
        return true;
        });
    this->ISys().SendTask("Main", "MulNXMain", [this]()->bool {
        this->Main();
        return true;
        });
    return true;
}
void VirtualUser::Main() {
    this->Update();
    if (this->pInputSystem->CheckWithPack(MulNX::KeyCheckPack{ true,false,false,true,'O',1 })) {
        bool AutoRunning = this->Enabled.load(std::memory_order_acquire);
        this->Enabled.store(!AutoRunning, std::memory_order_release);
        this->ISys().LogWarning(std::format("自动化增强已{}", AutoRunning ? "关闭" : "开启"));
    }
    return;
}

void VirtualUser::ProcessMsg(MulNX::Message& Msg) {
    if (!this->Enabled.load(std::memory_order_acquire))return;
    switch (Msg.type) {
    case "Game/NewRound"_hash: {
        this->ISys().LogInfo("接收到新回合信息");
        break;
    }
    case "Core/Tick1"_hash: {
        // this->Debugger->AddInfo("一秒");
        break;
    }
    case "Core/Tick60"_hash: {
        this->ISys().PublishAsync("Global/Save"_hash);
        this->ISys().LogInfo("已触发自动保存（频率：每分钟）");
        break;
    }
#ifdef _DEBUG
    case "Core/Tick30min"_hash: {
        this->ISys().AsyncCommand("playdemo 111");
        break;
    }
#endif // _DEBUG
    case "Command/SpecPlayer"_hash: {
        this->Core->ModuleManager()->FindModule<CSController>("CSController")->SpecPlayer(Msg.p1.low<int>());
        break;
    }
    default: {
        break;
    }

    }
    return;
}