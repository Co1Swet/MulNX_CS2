#include "CoreStarterBase.hpp"

#include <MulNX/Common/Message.hpp>
#include <MulNX/Core/Core.hpp>
#include <MulNX/Core/ModuleManager/ModuleManager.hpp>
#include <MulNX/Systems/I18nManager/I18nManager.hpp>
#include <MulNX/Systems/GlobalVars/GlobalVars.hpp>
#include <MulNX/Systems/TaskSystem/TaskSystem.hpp>
#include <MulNX/Systems/MessageManager/MessageManager.hpp>
#include <MulNX/Systems/Debugger/Debugger.hpp>
#include <MulNX/Systems/Logger/Logger.hpp>

bool MulNX::Core::CoreStarterBase::SystemInit(MulNX::Core::Core* pCore) {
    // 一阶段初始化核心启动器
    this->BaseInit(pCore);
    // 一阶段初始化模块管理器
    this->Core->ModuleManager()->SetName("ModuleManager");
    this->Core->ModuleManager()->BaseInit(this->Core);
    // 一阶段初始化注册模块
    this->Core->ModuleManager()->ModulesBaseInit();
    // 二阶段初始化模块管理器
    this->Core->ModuleManager()->EntryInit();
    // 二阶段初始化核心启动器
    this->EntryInit();
    // 二阶段初始化注册模块
    this->Core->ModuleManager()->ModulesInit();
    // 输出启动信息
    this->ISys().LogSucc(I18n("sys.started"));
    this->ISys().LogWarning(I18n("sys.version_is_testing", MulNXInfo::IsDebugVersion));
    this->ISys().LogWarning(I18n("sys.version_is", MulNXInfo::Version));
    this->ISys().LogWarning(I18n("sys.build_stamp", MulNXInfo::TimeStamp));
    // 执行启动器回调
    this->InitEndCall();
    // 记录结束时间
    auto end = std::chrono::steady_clock::now();
    // 输出总时间
    auto cost = std::chrono::duration_cast<std::chrono::microseconds>(end - this->Core->createTime);
    this->ISys().LogWarning(I18n("sys.inited_time_sum", cost.count()));
    return true;
}

void MulNX::Core::CoreStarterBase::CreateMainDraw() {
    // UI系统主界面初始化
    auto [msg2, rp] = MulNX::Message::Create<std::string>("UISystem/Start"_hash, "MainDraw");
    this->ISys().PublishAsync(std::move(msg2));
    this->ISys().LogWarning("发送了UI启动指令！渲染即将开始！");
}

void MulNX::Core::CoreStarterBase::CloseSystem() {
    // 设置系统标志位
    this->Core->ModuleManager()->FindModule<MulNX::GlobalVars>("GlobalVars")->SystemReady.store(false, std::memory_order_release);
    // 通知所有模块，以清理资源，包括线程停止
    this->Core->ModuleManager()->DeinitModules();
    // 任务系统汇合
    this->Core->ModuleManager()->FindModule<MulNX::TaskSystem>("TaskSystem")->Deinit();
    // 让日志最后一次打印
    this->Core->ModuleManager()->FindModule<MulNX::MessageManager>("MessageManager")->HandleDispatch();
    this->Core->ModuleManager()->FindModule<MulNX::Debugger>("Debugger")->Update();
    this->Core->ModuleManager()->FindModule<MulNX::Logger>("Logger")->Log();
    // 析构所有模块
    this->Core->ModuleManager()->Deinit();
    // 清理自身资源
    this->Deinit();
}