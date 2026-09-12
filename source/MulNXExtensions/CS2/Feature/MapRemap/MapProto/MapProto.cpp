#include "MapProto.hpp"

bool MapProto::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/engine2.dll", [this](auto&&...) {
        auto textRegion = this->CS2->engine2.GetTextRegion();
        this->OnEngine2Load(textRegion);
        });

    return true;
}

void MapProto::OnEngine2Load(MulNX::Memory::Region& textRegion) {
    // ─────────────────────────────────────────────────────────────────
    // DemoFileHeader 字符串公共出口：改 map_name
    // ─────────────────────────────────────────────────────────────────
    this->hkCDemoFileHeader_PraseString = MulNX::Hook::Create(
        textRegion.FindRegion(MulNX::CS2::Signatures::MapRemap::Protobuf::Pos_DemoFileHeader_PraseString).Data(),   // mov rbx, rax（公共出口）
        [this](MulNX::Hook* hk, RegContext* ctx) {
            uint64_t pHeader = ctx->rsi;   // = CDemoFileHeader + 8
            if (!pHeader) return MulNX::Hook::Then::Continue;
            // 过滤：只处理 map_name 字段（_has_bits_ bit 3）
            uint32_t hasBits = *(uint32_t*)(pHeader + 0x10);
            if ((hasBits & 0b1000) == 0) return MulNX::Hook::Then::Continue;
            // map_name 字段槽在 pHeader + 0x30
            CS2::CUtlStringRef ref((uint64_t*)(pHeader + 0x30));
            auto sv = ref.View();
            this->LogInfo(std::format("[DemoHeader/map_name] {}", sv));
            // 幂等：只替换 de_inferno
            if (sv == "de_inferno") {
                ref.Assign("de_mirage");
                this->LogInfo("[DemoHeader/map_name] de_inferno → de_mirage");
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
                name.Assign("de_mirage");
                this->LogInfo("[SpawnGroup/Name] → de_mirage");
            }
            if (sv == "maps/prefabs/de_inferno/inferno_skybox") {
                name.Assign("maps/prefabs/de_mirage/3dskybox_mirage_legacy");
            }

            return MulNX::Hook::Then::Continue;
        }, true).value();
    this->RegisterAttachHook(this->hkCNETMsg_SpawnGroup_Load_PraseString, "CNETMsg_SpawnGroup_Load_PraseString");
}