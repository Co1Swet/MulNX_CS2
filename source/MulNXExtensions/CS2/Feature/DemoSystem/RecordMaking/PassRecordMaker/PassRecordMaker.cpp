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

    return true;
}

void PassRecordMaker::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) { 
    case "Demo/Record/Make/Auto"_hash: {
        this->EnsureMake("kGAMEDEMOS", "111").Fire();
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
    

    co_return;
}