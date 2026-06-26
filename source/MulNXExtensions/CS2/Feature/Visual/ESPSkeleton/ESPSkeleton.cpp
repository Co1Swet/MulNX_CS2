#include "ESPSkeleton.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <Intro/HookView/HookView.hpp>

void ESPSkeleton::DrawSkelgton(CS2::C_CSPlayerPawn* pPawn) {
    for (const auto& chain : chains) {
        for (size_t i = 0; i + 1 < chain.size(); ++i) {
            auto iStart = chain[i];
            auto iEnd = chain[i + 1];
            auto start = pPawn->GetBonePos(iStart);
            auto end = pPawn->GetBonePos(iEnd);
            MulNX::TransInfo info;
            info.pMatrix = this->CS2View->GetViewMatrix();
            info.windowHeight = this->CS2View->GetWinHeight();
            info.windowWidth = this->CS2View->GetWinWidth();
            MulNX::UI::DrawWorldLine(start, end, info, IM_COL32(255, 0, 0, 255));
        }
    }
}

void ESPSkeleton::Draw(MulNX::UINode* node) {
    if (!this->runFlag1.load())return;
    try {
        for (int i = 0;i < this->CS2->client.dwGameEntitySystem_highestEntityIndex() && i < 30;++i) {
            auto pController = this->CS2->client.GetBaseEntity(i)->As<CS2::CCSPlayerController>();
            if (!pController)continue;
            auto hPawn = MulNX::MRead(pController->m_hPlayerPawn());
            auto pPawn = this->CS2->client.GetBaseEntityFromHandle(hPawn)->As<CS2::C_CSPlayerPawn>();
            if (!pPawn)continue;
            this->DrawSkelgton(pPawn);
        }
    }
    catch (...) {
        return;
    }
    return;
}

bool ESPSkeleton::Init() {
    auto pthConfig = this->PathGet("Config") / "Config.yaml";

    YAML::Node fConfig = YAML::LoadFile(pthConfig.string());
    
    for (const auto& chain : fConfig) {
        std::vector<int> indices;
        for (const auto& idx : chain)
            indices.push_back(idx.as<int>());
        if (indices.size() >= 2)
            this->chains.push_back(std::move(indices));
    }

    this->SendUIRoot("DrawSkeleton", [this](MulNX::UINode* node) {return this->Draw(node);});
    this->SendUINode(this->GetName(), [this](MulNX::UINode* node) {
        MulNX::UI::Checkbox("骨骼绘制", this->runFlag1);
        });

    return true;
}