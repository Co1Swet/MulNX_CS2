#include "MapKeyValues.hpp"

bool MapKeyValues::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](auto&&...) {
        auto textRegion = this->CS2->client.GetTextRegion();
        this->OnClientLoad(textRegion);
        });

    return true;
}

void MapKeyValues::OnClientLoad(MulNX::Memory::Region& textRegion) {
    auto target = textRegion.FindRegion(MulNX::CS2::Signatures::MapRemap::Pos_MapKVReaded).Data();
    this->hkPos_MapKVReaded = MulNX::Hook::Create(target, [this](MulNX::Hook* hk, RegContext* ctx) {
        uint64_t pGSC = (uint64_t)ctx->rdi;
        auto pKeyValues = reinterpret_cast<CS2::KeyValues*>(ctx->rbp);
        this->LogInfo(std::format("KeyValues 首次创建 a2={:#x} pKV={:#x}", pGSC, (uint64_t)pKeyValues));
        // 确认 "map" 键存在
        uint64_t pMapChk = pKeyValues->FindKey("map", false);
        uint64_t pLevelChk = pKeyValues->FindKey("levelname", false);
        this->LogInfo(std::format("FindKey('map')={:#x} FindKey('levelname')={:#x}", pMapChk, pLevelChk));
        // 在根 KeyValues上SetString
        pKeyValues->SetString("map", "de_inferno_rain");
        this->LogInfo("SetString(root, 'map', 'de_inferno_rain')");
        pKeyValues->SetString("levelname", "de_inferno_rain");
        this->LogInfo("SetString(root, 'levelname', 'de_inferno_rain')");
        // 读回验证
        uint64_t pMapAfter = pKeyValues->FindKey("map", false);
        this->LogInfo(std::format("改后 FindKey('map')={:#x}", pMapAfter));

        return MulNX::Hook::Then::Continue;
        }, true).value();
    this->RegisterAttachHook(this->hkPos_MapKVReaded, "Pos_MapKVReaded");
}