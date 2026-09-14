#include "MapProto.hpp"

bool MapProto::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/engine2.dll", [this](auto&&...) {
        auto textRegion = this->CS2->engine2.GetTextRegion();
        this->OnEngine2Load(textRegion);
        });

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
            if (hasBits & 0b1000) {
                CS2::CUtlStringRef ref((uint64_t*)(pHeader + 0x30));
                auto sv = ref.View();
                this->LogInfo(std::format("[DemoHeader/map_name] '{}'", sv));
                if (sv == "de_inferno") {
                    ref.Assign("de_inferno_rain");
                    this->LogInfo("[DemoHeader/map_name] de_inferno → de_inferno_rain");
                }
            }
            // field 10: addons
            if (hasBits & 0b100000) {
                CS2::CUtlStringRef ref((uint64_t*)(pHeader + 0x40));
                auto sv = ref.View();
                this->LogInfo(std::format("[DemoHeader/addons] '{}' (len={})", sv, sv.size()));

                uint64_t tagged = *(uint64_t*)(pHeader + 0x40);
                uint64_t tag = tagged & 3;
                this->LogInfo(std::format("[DemoHeader/addons] tagged={:#x} tag={} pStd={:#x}",
                    tagged, tag, tagged & ~3ull));

                ref.Assign("3488478228");
            }

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

            this->LogInfo(std::format("[SpawnGroup/Name] {}", sv));

            if (sv == "de_inferno") {
                name.Assign("de_inferno_rain");
                this->LogInfo("[SpawnGroup/Name] → de_inferno_rain");
            }
            if (sv == "maps/prefabs/de_inferno/inferno_skybox") {
                //name.Assign("maps/prefabs/de_inferno_rain/3dskybox_mirage_legacy");
            }

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
            this->LogInfo(std::format("[CAT/mapname] {}", sv));
            if (sv == "de_inferno") {
                name.Assign("de_inferno_rain");
                this->LogInfo("[CAT/mapname] de_inferno → de_inferno_rain");
            }
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
            if (sv == "de_inferno") {
                ref.Assign("de_inferno_rain");
                this->LogInfo("[SSI/map_name] de_inferno → de_inferno_rain");
            }
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

            this->LogInfo(std::format("[GSC/s1_mapname] {}", sv));

            if (sv == "de_inferno") {
                ref.Assign("de_inferno_rain");
                this->LogInfo("[GSC/s1_mapname] de_inferno → de_inferno_rain");
            }

            return MulNX::Hook::Then::Continue;
        }, true).value();
    this->RegisterAttachHook(this->hkCSVCMsg_GameSessionConfiguration_PraseString,
        "CSVCMsg_GameSessionConfiguration_PraseString");
}