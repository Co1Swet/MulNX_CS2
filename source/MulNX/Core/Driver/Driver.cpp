#include "Driver.hpp"
#include <MulNX/Common/Message.hpp>
#include <MulNX/Core/ModuleManager/ModuleManager.hpp>
#include <MulNX/Systems/Systems.hpp>

MulNX::Core::Driver::Driver() {
    this->preInits.push_back([this]() {
        this->LogSucc("驱动就绪");
        return true;
        });
    this->postInits.clear();
    this->postDeinits.clear();
}
bool MulNX::Core::Driver::Init() {
    std::unique_lock lock(this->endMtx);
    this->SubscribeSync("Driver/End/Notify", [this](auto&&...) {
        std::unique_lock lock(this->endMtx);
        this->endFlag = true;
        this->cv.notify_one();
        });
    // 初始化模块管理器
    this->Core->ModuleManager()->EntryInit(this->Core);
    // 初始化注册模块
    this->Core->ModuleManager()->ModulesInit();
    this->PublishSync("System/Init/End"_hash);
    // 输出启动信息
    this->LogSucc(I18n("sys.started"));
    this->LogWarning(I18n("sys.version_is_testing", MulNXInfo::IsDebugVersion));
    this->LogWarning(I18n("sys.version_is", MulNXInfo::Version));
    this->LogWarning(I18n("sys.build_stamp", MulNXInfo::TimeStamp));
    // 记录结束时间
    auto end = std::chrono::steady_clock::now();
    // 输出总时间
    auto cost = std::chrono::duration_cast<std::chrono::microseconds>(end - this->Core->createTime);
    this->LogWarning(I18n("sys.inited_time_sum", cost.count()));
    this->LogWarning(I18n("disclaimer"));
    return true;
}
void MulNX::Core::Driver::WaitEnd() {
    std::unique_lock lock(this->endMtx);
    this->cv.wait(lock, [this] { return this->endFlag; });
    // 设置系统标志位
    this->Core->ModuleManager()->FindModule<MulNX::GlobalVars>("GlobalVars")->SystemReady.store(false, std::memory_order_release);
    // 通知所有模块，以清理资源，包括线程停止
    this->Core->ModuleManager()->DeinitModules();
    // 任务系统汇合
    this->Core->ModuleManager()->FindModule<MulNX::TaskSystem>("TaskSystem")->EntryDeinit();
    // 让日志最后一次打印
    this->Core->ModuleManager()->FindModule<MulNX::MessageManager>("MessageManager")->HandleDispatch();
    this->Core->ModuleManager()->FindModule<MulNX::Logger>("Logger")->Log();
    // 析构所有模块
    this->Core->ModuleManager()->EntryDeinit();
    // 清理自身资源
    this->EntryDeinit();
}