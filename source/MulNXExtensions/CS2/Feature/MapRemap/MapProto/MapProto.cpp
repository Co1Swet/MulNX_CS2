#include "MapProto.hpp"

bool MapProto::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/engine2.dll", [this](auto&&...) {
        auto textRegion = this->CS2->engine2.GetTextRegion();
        this->OnEngine2Load(textRegion);
        });

    return true;
}

bool MapProto::CheckName(std::string_view sv) {
    if (sv == "<empty>")return false;
    auto pRawName = this->pMapState->pRawMapName.load();
    if (!pRawName)return false;
    if (*pRawName != sv)return false;
    return true;
}

void MapProto::OnEngine2Load(MulNX::Memory::Region& textRegion) {
    // DemoFileHeader 字符串公共出口
    // target: mov rbx, rax
    // rsi = pHeader = pMessage = pDemoFileHeader + 8
    // _has_bits_ rsi +0x10
    this->hkCDemoFileHeader_PraseString = MulNX::Hook::Create(
        textRegion.FindRegion(MulNX::CS2::Signatures::MapRemap::Protobuf::Pos_DemoFileHeader_PraseString).Data(),
        [this](MulNX::Hook* hk, RegContext* ctx) {
            uint64_t pHeader = ctx->rsi;
            if (!pHeader) return MulNX::Hook::Then::Continue;
            uint32_t hasBits = *(uint32_t*)(pHeader + 0x10);
            // field 5: map_name
            [&]()->void {
                if (!(hasBits & 0b1000))return;
                CS2::CUtlStringRef ref((uint64_t*)(pHeader + 0x30));
                auto sv = ref.View();
                if (!CheckName(sv))return;
                auto pName = this->pMapState->pTargetMapName.load();
                if (!pName)return;
                this->LogInfo(std::format("[DemoHeader/map_name] {} → {}", sv, pName->c_str()));
                ref.Assign(pName->c_str(), pName->size());
                }();
            // field 10: addons
            [&]()->void {
                if (!(hasBits & 0b100000))return;
                CS2::CUtlStringRef ref((uint64_t*)(pHeader + 0x40));
                auto sv = ref.View();
                auto pAddon = this->pMapState->pTargetAddonID.load();
                if (!pAddon)return;
                if (sv == *pAddon)return;
                this->LogInfo(std::format("[DemoHeader/addons] {} -> {}", sv, pAddon->c_str()));
                ref.Assign(pAddon->c_str(), pAddon->size());
                }();
            return MulNX::Hook::Then::Continue;
        }, true).value();
    this->RegisterAttachHook(this->hkCDemoFileHeader_PraseString, "CDemoFileHeader_PraseString");

    // SpawnGroup 字符串公共出口：改 name（数据源 1/2）
    this->hkCNETMsg_SpawnGroup_Load_PraseString = MulNX::Hook::Create(
        textRegion.FindRegion(MulNX::CS2::Signatures::MapRemap::Protobuf::Pos_CNETMsg_SpawnGroup_Load_PraseString).Data(),
        [this](MulNX::Hook* hk, RegContext* ctx) {
            auto a1 = (uint8_t*)ctx->rsi;
            if (!a1) return MulNX::Hook::Then::Continue;
            if (!ctx->rax) return MulNX::Hook::Then::Continue;

            uint32_t hasBits = *(uint32_t*)(a1 + 0x10);
            if ((hasBits & 0b1) == 0) return MulNX::Hook::Then::Continue;

            CS2::CUtlStringRef name((uint64_t*)(a1 + 0x18));
            std::string_view sv = name.View();
            if (sv.empty()) return MulNX::Hook::Then::Continue;
            if (!CheckName(sv))return MulNX::Hook::Then::Continue;

            auto pName = this->pMapState->pTargetMapName.load();
            if (!pName)return MulNX::Hook::Then::Continue;
            this->LogInfo(std::format("[SpawnGroup/Name] {} → {}", sv, pName->c_str()));
            name.Assign(pName->c_str(), pName->size());
            // if (sv == "maps/prefabs/de_inferno/inferno_skybox") {
            //     name.Assign("maps/prefabs/de_inferno_rain/3dskybox_mirage_legacy");
            // }
            return MulNX::Hook::Then::Continue;
        }, true).value();
    this->RegisterAttachHook(this->hkCNETMsg_SpawnGroup_Load_PraseString, "CNETMsg_SpawnGroup_Load_PraseString");

    // CSVCMsg_ClearAllStringTables 字符串写入点
    this->hkCSVCMsg_ClearAllStringTables_PraseString = MulNX::Hook::Create(
        textRegion.FindRegion(MulNX::CS2::Signatures::MapRemap::Protobuf::Pos_CSVCMsg_ClearAllStringTables_PraseString).Data(),
        [this](MulNX::Hook* hk, RegContext* ctx) {
            uint64_t a1 = (uint64_t)ctx->rsi;
            if (!a1) return MulNX::Hook::Then::Continue;
            CS2::CUtlStringRef name((uint64_t*)(a1 + 0x18));
            auto sv = name.View();
            if (!CheckName(sv))return MulNX::Hook::Then::Continue;
            auto pName = this->pMapState->pTargetMapName.load();
            if (!pName)return MulNX::Hook::Then::Continue;
            if (sv == *pName)return MulNX::Hook::Then::Continue;
                
            name.Assign(pName->c_str(), pName->size());
            this->LogInfo(std::format("[CAT/mapname] {} → {}", sv, pName->c_str()));
            
            return MulNX::Hook::Then::Continue;
        }, true).value();
    this->RegisterAttachHook(this->hkCSVCMsg_ClearAllStringTables_PraseString, "CSVCMsg_ClearAllStringTables");

    // CSVCMsg_ServerInfo 字符串公共出口：改 map_name（rsi + 0x20）
    this->hkCSVCMsg_ServerInfo_PraseString = MulNX::Hook::Create(
        textRegion.FindRegion(MulNX::CS2::Signatures::MapRemap::Protobuf::Pos_CSVCMsg_ServerInfo_PraseString).Data(),
        [this](MulNX::Hook* hk, RegContext* ctx) {
            auto rsi = (uint64_t)ctx->rsi;
            if (!rsi) return MulNX::Hook::Then::Continue;
            // 检查 map_name 槽（rsi + 0x20）
            CS2::CUtlStringRef ref((uint64_t*)(rsi + 0x20));
            std::string_view sv = ref.View();
            if (!CheckName(sv))return MulNX::Hook::Then::Continue;
            auto pName = this->pMapState->pTargetMapName.load();
            if (!pName)return MulNX::Hook::Then::Continue;
            if (sv == *pName)return MulNX::Hook::Then::Continue;
                
            ref.Assign(pName->c_str(), pName->size());
            this->LogInfo(std::format("[SSI/map_name] {} → {}", sv, pName->c_str()));
            
            return MulNX::Hook::Then::Continue;
        }, true).value();
    this->RegisterAttachHook(this->hkCSVCMsg_ServerInfo_PraseString, "CSVCMsg_ServerInfo_PraseString");

    // CSVCMsg_GameSessionConfiguration 字符串公共出口：改 s1_mapname（field 11）
    // RVA: 0x17B7A3 (mov rbx, rax)
    // rsi = a1 = pGSC
    // _has_bits_ 低位在 +0x10，field 11 是 bit 2
    // field 11 slot 在 a1 + 0x28
    this->hkCSVCMsg_GameSessionConfiguration_PraseString = MulNX::Hook::Create(
        textRegion.FindRegion(MulNX::CS2::Signatures::MapRemap::Protobuf::Pos_CSVCMsg_GameSessionConfiguration_PraseString).Data(),
        [this](MulNX::Hook* hk, RegContext* ctx) {
            auto a1 = (uint8_t*)ctx->rsi;
            if (!a1) return MulNX::Hook::Then::Continue;

            // 过滤：只处理 s1_mapname 字段（_has_bits_ bit 2）
            uint32_t hasBits = *(uint32_t*)(a1 + 0x10);
            if ((hasBits & 0b100) == 0) return MulNX::Hook::Then::Continue;

            CS2::CUtlStringRef ref((uint64_t*)(a1 + 0x28));
            std::string_view sv = ref.View();
            if (sv.empty()) return MulNX::Hook::Then::Continue;
            if (!CheckName(sv))return MulNX::Hook::Then::Continue;
            auto pName = this->pMapState->pTargetMapName.load();
            if (!pName)return MulNX::Hook::Then::Continue;
            if (sv == *pName)return MulNX::Hook::Then::Continue;
            ref.Assign(pName->c_str(), pName->size());
            this->LogInfo(std::format("[GSC/s1_mapname] {} → {}", sv, pName->c_str()));
            return MulNX::Hook::Then::Continue;
        }, true).value();
    this->RegisterAttachHook(this->hkCSVCMsg_GameSessionConfiguration_PraseString,
        "CSVCMsg_GameSessionConfiguration_PraseString");
}