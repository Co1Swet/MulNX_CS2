#include "MapKeyValues.hpp"

bool MapKeyValues::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](auto&&...) {
        auto textRegion = this->CS2->client.GetTextRegion();
        this->OnClientLoad(textRegion);
        });

    return true;
}

void MapKeyValues::OnClientLoad(MulNX::Memory::Region& textRegion) {
    auto target = textRegion.FindRegion(CS2::Signatures::MapRemap::Pos_MapKVReaded).Data();
    this->hkPos_MapKVReaded = MulNX::Hook::Create(target, [this](MulNX::Hook* hk, RegContext* ctx) {
        auto pTargetName = this->pMapState->pTargetMapName.load(std::memory_order_acquire);
        if (!pTargetName)return MulNX::Hook::Then::Continue;
        uint64_t pGSC = (uint64_t)ctx->rdi;
        auto pKeyValues = reinterpret_cast<CS2::KeyValues*>(ctx->rbp);
        // 在根 KeyValues上SetString
        pKeyValues->SetString("map", pTargetName->c_str());
        pKeyValues->SetString("levelname", pTargetName->c_str());
        this->LogInfo(std::format("KeyValues键map levelname值修改为：{}", pTargetName->c_str()));
        return MulNX::Hook::Then::Continue;
        }, true).value();
    this->RegisterAttachHook(this->hkPos_MapKVReaded, "Pos_MapKVReaded");
}