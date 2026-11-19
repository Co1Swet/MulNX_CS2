#include "PassRecordMaker.hpp"
#include <MulNX/Base/fs.hpp>
#include <Intro/HookConsole/HookConsole.hpp>
#include <Feature/DemoSystem/RecordMaking/AutoRecordRule/AutoRecordRule.hpp>

bool PassRecordMaker::Init() {
    this->pAutoRecordRule = this->FindModule<AutoRecordRule>();

    (*this)
        .SubscribeAsyncEx<void>("Demo/Record/Make/Auto")
        .SubscribeAsyncEx<int>("Demo/Info/Notify")
        .SubscribeAsyncEx<int>("CS/LocalPlayerUID")
        ;

    this->SendTask("Update", "DemoSys", [this]() {
        this->Update();
        return true;
        });

    return true;
}

void PassRecordMaker::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) { 
    case "Demo/Record/Make/Auto"_hash: {
        auto r = this->ParseDemoPath(msg);
        if (!r) {
            auto&& [key, name, error] = r.error();
            this->LogError(std::format("无法根据规则自动生成 {}\\{}：{}", key, name, error));
            break;
        }
        auto&& [key, name, path] = *r;
        this->EnsureMake(key, name).Fire();
        break;
    }
    case "CS/LocalPlayerUID"_hash: {
        auto&& [uid] = msg.Access<Steam64UID>();
        this->OnUID(uid).Fire();
        break;
    }
    }
}

MulNX::CoTask PassRecordMaker::AsyncWaitInfo(bool& succ, const std::string& key, const std::string& demoName) {
    auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/Analyze"_hash);
    rp->str1 = key;
    rp->str2 = demoName;
    this->PublishAsync(std::move(msg));

    while (true) {
        MulNX::Message* pMsg = nullptr;
        co_await this->WaitMsgForever("Demo/Info/Notify"_hash, pMsg);
        auto r = this->ParseDemoPath(*pMsg);
        if (!r)continue;
        auto&& [kDir, name, path] = *r;
        if (kDir != key)continue;
        if (name != demoName)continue;

        auto&& [suc] = pMsg->Access<int>();
        succ = (suc == 0);
        co_return;
    }
}

MulNX::CoTask PassRecordMaker::EnsureMake(std::string key, std::string demoName, std::optional<Steam64UID> localSteamUID) {
    auto p = this->pDemSys->GetDemoInfo(key, demoName);
    if (!p) {
        this->LogInfo(std::format("Demo {} 的信息尚未加载，即将发送分析请求", demoName));
        bool ok = false;
        co_await this->AsyncWaitInfo(ok, key, demoName);
        if (!ok) {
            this->LogError(std::format("Demo分析失败，无法执行自动化：{}", demoName));
            co_return;
        }
    }
    p = this->pDemSys->GetDemoInfo(key, demoName);
    if (!p) {
        this->LogError(std::format("理论上 {} 已经被分析，但是无法得到有效信息，已丢弃自动生成", demoName));
        co_return;
    }

    auto plan = this->pAutoRecordRule->BuildPlan(*p);
    if (plan.requests.empty()) {
        this->LogWarning(std::format("Demo {} 没有符合自动录制规则的请求", demoName));
        co_return;
    }

    for (auto& request : plan.requests) {
        this->PublishAsync(std::move(request));
    }

    MulNX::Message barrier("Demo/Record/Commit/Demo"_hash);
    auto&& [autoRecord] = barrier.Access<bool>();
    autoRecord = plan.autoStart || localSteamUID.has_value();
    this->PublishAsync(std::move(barrier));

    this->LogInfo(std::format(
        "自动规则为 Demo {} 发送 {} 条录制请求，提交后启动录制：{}",
        demoName, plan.requests.size(), plan.autoStart ? "是" : "否"));

    co_return;
}

MulNX::CoTask PassRecordMaker::OnUID(Steam64UID uid) {
    co_await this->WaitMilliseconds(5000);
    auto path = this->Path()->PathGetForShared("DemoInput") / "LocalPlayer";
    auto dems = fs::SubFileNames(path, { ".dem" });
    if (!dems.has_value()) {
        this->LogError(dems.error().message());
        co_return;
    }

    co_return;
}