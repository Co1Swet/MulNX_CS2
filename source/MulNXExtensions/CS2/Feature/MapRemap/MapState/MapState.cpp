#include "MapState.hpp"

void MapState::Menu() {
    static std::string targetAddonID{};
    ImGui::InputText("目标Addon号", &targetAddonID);
    static std::string targetMapName{};
    ImGui::InputText("目标地图名", &targetMapName);

    if (ImGui::Button("设置")) {
        if (targetAddonID.empty() || targetMapName.empty()) {
            this->LogError("Addon号或地图名不能为空！");
            return;
        }
        auto [msg, rp] = MulNX::Message::Create<MulNX::NetExt>("MapRemap/Set"_hash);
        rp->str1 = targetAddonID;
        rp->str2 = targetMapName;
        this->PublishAsync(std::move(msg));
    }
}

bool MapState::Init() {
    this->pRawMapName = std::make_shared<std::string>("de_inferno");
    this->pTargetAddonID = std::make_shared<std::string>("3488478228");
    this->pTargetMapName = std::make_shared<std::string>("de_inferno_rain");

    (*this)
        .SubscribeAsync("MapRemap/Set")
        ;

    this->UIRegisterCallback("UI.3DVision", [this](auto&&...) {
        this->Menu();
        });

    this->SendTask("Update", "CSControl", [this] {
        this->Update();
        return true;
        });

    return true;
}
void MapState::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "MapRemap/Set"_hash: {
        auto pNetExt = msg.asp.get<MulNX::NetExt>();
        this->pTargetAddonID = std::make_shared<std::string>(pNetExt->str1);
        this->pTargetMapName = std::make_shared<std::string>(pNetExt->str2);
        break;
    }
    }
}