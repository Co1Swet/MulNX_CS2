#include "PassRecordMaker.hpp"

bool PassRecordMaker::Init() {

    (*this)
        .SubscribeAsyncEx<void>("Demo/Record/Make/Auto")
        .SubscribeAsyncEx<void>("Demo/Record/Make/Auto")
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
        this->EnsureMake("test123").Fire();
        break;
    }
    }
}

MulNX::CoTask PassRecordMaker::EnsureMake(std::string&& demoName) {

    auto p = this->pDemSys->GetDemoInfo(demoName);
    if (!p) {
        this->LogInfo(std::format("Demo {} 的信息尚未加载", demoName));
    }

    co_return;
}