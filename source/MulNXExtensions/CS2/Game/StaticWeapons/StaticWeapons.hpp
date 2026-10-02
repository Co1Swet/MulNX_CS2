#pragma once
#include "Grenades.hpp"
#include "Miscs.hpp"
#include "Pistols.hpp"
#include "Rifles.hpp"
#include "Shotguns.hpp"
#include "SMGs.hpp"
#include "Snipers.hpp"

namespace CS2 {
    using namespace StaticWeapons;

    inline constinit const auto kAllWeapons = std::to_array<WeaponDescriptor>({
        // 步枪 (7)
        AK47::descriptor(), AUG::descriptor(), FAMAS::descriptor(), GalilAR::descriptor(),
        M4A1::descriptor(), M4A1Silencer::descriptor(), SG556::descriptor(),
        // 狙击 (4)
        AWP::descriptor(), SSG08::descriptor(), SCAR20::descriptor(), G3SG1::descriptor(),
        // SMG (7)
        MP9::descriptor(), MAC10::descriptor(), MP7::descriptor(), MP5SD::descriptor(),
        UMP45::descriptor(), Bizon::descriptor(), P90::descriptor(),
        // 手枪 (10)
        Glock::descriptor(), USPSilencer::descriptor(), HKP2000::descriptor(),
        P250::descriptor(), FiveSeven::descriptor(), Deagle::descriptor(),
        Revolver::descriptor(), Elite::descriptor(), Tec9::descriptor(), CZ75A::descriptor(),
        // 霰弹枪 (4)
        Nova::descriptor(), MAG7::descriptor(), SawedOff::descriptor(), XM1014::descriptor(),
        // 机枪 (2)
        M249::descriptor(), Negev::descriptor(),
        // 刀 (2)
        Knife::descriptor(), Bayonet::descriptor(),
        // 手雷 / 投掷物 (6)
        HEGrenade::descriptor(), Molotov::descriptor(), IncGrenade::descriptor(),
        Flashbang::descriptor(), SmokeGrenade::descriptor(), Decoy::descriptor(),
        // 电击枪 (1)
        Taser::descriptor(),
        // C4 (2)
        C4::descriptor(), PlantedC4::descriptor(),
        // 特殊来源 (3)
        World::descriptor(), WorldSpawn::descriptor(), Inferno::descriptor(),
        });

    // 查找：三级匹配，线性扫描
    namespace detail {
        inline std::string_view Normalize(std::string_view raw) noexcept {
            if (raw.size() > 7 && raw.substr(0, 7) == "weapon_")
                raw.remove_prefix(7);
            return raw;
        }
        inline bool IEquals(std::string_view a, std::string_view b) noexcept {
            if (a.size() != b.size()) return false;
            for (size_t i = 0; i < a.size(); ++i)
                if (std::tolower(static_cast<unsigned char>(a[i])) !=
                    std::tolower(static_cast<unsigned char>(b[i])))
                    return false;
            return true;
        }
        inline const WeaponDescriptor* ExactMatch(std::string_view name) noexcept {
            for (const auto& w : kAllWeapons)
                if (w.canonical == name) return &w;
            return nullptr;
        }
        inline const WeaponDescriptor* CaseInsensitiveMatch(std::string_view name) noexcept {
            for (const auto& w : kAllWeapons)
                if (IEquals(w.canonical, name)) return &w;
            return nullptr;
        }
    }
    // 查找武器描述符：
    // 剥 weapon_ 前缀
    // knife*/bayonet* 前缀归类到基础名
    // 大小写不敏感兜底
    inline const WeaponDescriptor* FindWeapon(std::string_view raw) noexcept {
        if (raw.empty()) return nullptr;
        auto name = detail::Normalize(raw);

        if (const auto* w = detail::ExactMatch(name)) return w;

        if (name.size() >= 5 && name.substr(0, 5) == "knife")
            if (const auto* w = detail::ExactMatch("knife")) return w;
        if (name.size() >= 7 && name.substr(0, 7) == "bayonet")
            if (const auto* w = detail::ExactMatch("bayonet")) return w;

        return detail::CaseInsensitiveMatch(name);
    }
}