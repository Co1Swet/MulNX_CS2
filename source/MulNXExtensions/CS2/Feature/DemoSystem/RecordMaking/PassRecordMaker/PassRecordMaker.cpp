#include "PassRecordMaker.hpp"

bool PassRecordMaker::Init() {

    (*this)
        .SubscribeAsyncEx<void>("Demo/Record/Make/Auto")
        .SubscribeAsyncEx<int>("Demo/Info/Notify")
        ;

    this->SendTask("Update", "DemoSys", [this]() {
        this->Update();
        return true;
        });

    this->PublishAsync("Demo/Record/Make/Auto"_hash);

    return true;
}

void PassRecordMaker::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Demo/Record/Make/Auto"_hash: {
        this->EnsureMake("111").Fire();
        break;
    }
    }
}

MulNX::CoTask PassRecordMaker::AsyncWaitInfo(bool& succ, const std::string& demoName) {
    while (true) {
        MulNX::Message* pMsg = nullptr;
        co_await this->WaitMsgForever("Demo/Info/Notify"_hash, pMsg);
        auto pNet = pMsg->asp.get<MulNX::NetExt>();
        const auto& name = pNet->str1;
        if (name != demoName)continue;

        auto&& [suc] = pMsg->Access<int>();
        succ = (suc == 0);
        co_return;
    }
}

MulNX::CoTask PassRecordMaker::EnsureMake(std::string&& n) {
    auto demoName = std::move(n);

    auto p = this->pDemSys->GetDemoInfo(demoName);
    if (!p) {
        this->LogInfo(std::format("Demo {} 的信息尚未加载，即将发送分析请求", demoName));

        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("Demo/Analyze"_hash);
        rp->str1 = (this->CS2Paths->demo / (demoName + ".dem")).string();
        this->PublishAsync(std::move(msg));
        bool ok = false;
        co_await this->AsyncWaitInfo(ok, demoName);
        if (!ok) {
            this->LogError(std::format("Demo分析失败，无法执行自动化：{}", demoName));
            co_return;
        }
    }
    p = this->pDemSys->GetDemoInfo(demoName);
    if (!p) {
        this->LogError(std::format("理论上 {} 已经被分析，但是无法得到有效信息，已丢弃自动生成", demoName));
        co_return;
    }

    co_return;
}