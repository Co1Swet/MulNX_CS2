#include "PassRecordMaker.hpp"
#include <Intro/HookConsole/HookConsole.hpp>
#include <Feature/DemoSystem/RecordMaking/AutoRecordRule/AutoRecordRule.hpp>

bool PassRecordMaker::Init() {
    this->pAutoRecordRule = this->FindModule<AutoRecordRule>("AutoRecordRule");

    (*this)
        .SubscribeAsyncEx<void>("Demo/Record/Make/Auto")
        .SubscribeAsyncEx<int>("Demo/Info/Notify")
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

MulNX::CoTask PassRecordMaker::EnsureMake(std::string key, std::string demoName) {
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

    const auto requestCount = plan.requests.size();
    for (auto& request : plan.requests) {
        this->PublishAsync(std::move(request));
    }

    MulNX::Message barrier("Demo/Record/Commit/Demo"_hash);
    auto&& [autoRecord] = barrier.Access<int>();
    autoRecord = plan.autoStart ? 1 : 0;
    this->PublishAsync(std::move(barrier));

    this->LogInfo(std::format(
        "自动规则为 Demo {} 发送 {} 条录制请求，提交后{}启动录制",
        demoName, requestCount, plan.autoStart ? "将" : "不"));

    co_return;
}