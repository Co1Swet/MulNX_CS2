#include "SkinConfigger.hpp"

void SkinConfigger::Menu()const {
    std::shared_lock lock(this->smutex);

    ImGui::SeparatorText("模型配置");
    auto policy = this->policy.load(std::memory_order_acquire);
    ImGui::TextUnformatted("当前模式：");
    ImGui::SameLine();
    switch (policy) {
    case Policy::Disable: {
        ImGui::TextUnformatted("未激活");
        break;
    }
    case Policy::BuyNoOwn: {
        ImGui::TextUnformatted("仅购买者决定");
        break;
    }
    case Policy::BuyOwn: {
        ImGui::TextUnformatted("优先购买者，回退持有者");
        break;
    }
    case Policy::OwnNoBuy: {
        ImGui::TextUnformatted("仅持有者决定");
        break;
    }
    case Policy::OwnBuy: {
        ImGui::TextUnformatted("优先持有者，回退购买者");
        break;
    }
    }

    if (ImGui::Button("关闭功能")) {
        this->PublishAsync("SkinChange/Disable"_hash);
    }
    ImGui::SameLine();
    if (ImGui::Button("仅购买者决定")) {
        this->PublishAsync("SkinChange/BuyNoOwn"_hash);
    }
    ImGui::SameLine();
    if (ImGui::Button("优先购买者，回退持有者")) {
        this->PublishAsync("SkinChange/BuyOwn"_hash);
    }
    ImGui::SameLine();
    if (ImGui::Button("仅持有者决定")) {
        this->PublishAsync("SkinChange/OwnNoBuy"_hash);
    }
    ImGui::SameLine();
    if (ImGui::Button("优先持有者，回退购买者")) {
        this->PublishAsync("SkinChange/OwnBuy"_hash);
    }

    ImGui::TextUnformatted("当前配置：");

    if (this->cfgs.empty()) {
        ImGui::TextUnformatted("无任何配置");
    }
    for (const auto& [steamID, cfg] : this->cfgs) {
        if (!ImGui::CollapsingHeader(std::format("玩家：{}", steamID).c_str()))continue;
        for (const auto& [weaponName, playerWeaponCfg] : cfg) {
            ImGui::TextUnformatted(std::format("武器名：{}", weaponName).c_str());

            const auto fmtOpt = [](const auto& opt) -> std::string {
                return opt.has_value() ? std::format("{}", *opt) : "<未设置>";
                };
            ImGui::TextUnformatted(std::format(
                "--- 皮肤索引：{}，模板种子：{}，磨损度：{}",
                fmtOpt(playerWeaponCfg.paintKit),
                fmtOpt(playerWeaponCfg.seed),
                fmtOpt(playerWeaponCfg.wear)).c_str());
        }
    }

    ImGui::SeparatorText("操作");

    if (ImGui::Button("清除所有配置")) {
        this->PublishAsync("SkinChange/ClearAll"_hash);
    }

    static int paintKit{};
    static int seed{};
    static float wear{};
    static Steam64UID targetUID{};
    static std::string targetPlayerName{};
    static std::string targetWeaponName{};

    [&]() {
        auto pPawn = this->CS2Entitys->TryGetObservingPawn();
        if (!pPawn) {
            ImGui::TextUnformatted("当前未观战任何玩家");
            return;
        }
        auto pCtrl = this->CS2Entitys->GetBaseEntityFromHandle(MulNX::MRead(pPawn->m_hController()))->As<CS2::CCSPlayerController>();
        if (!pCtrl)return;
        auto steamID = MulNX::MRead(pCtrl->m_steamID());
        auto playerName = MulNX::Memory::ReadString(pCtrl->m_iszPlayerName()).value_or("读取失败");
        auto pWeaponSv = MulNX::MRead(pPawn->pWeaponServices());
        if (!pWeaponSv)return;
        auto hActive = MulNX::MRead(pWeaponSv->hActiveWeapon());
        auto pWeapon = this->CS2Entitys->GetBaseEntityFromHandle(hActive)->As<CS2::C_CSWeaponBase>();
        if (!pWeapon)return;
        auto weaponName = pWeapon->GetName();

        ImGui::TextUnformatted(std::format("当前正在观战：{} ，steamID：{} ，正持有的武器名：{}",
            playerName, steamID, weaponName).c_str());


        if (ImGui::Button("为此ID修改")) {
            targetUID = steamID;
            targetPlayerName = playerName;
            targetWeaponName = weaponName;
            paintKit = {};
            seed = {};
            wear = {};
        }
        }();

    if (targetUID == 0) {
        ImGui::TextUnformatted("未选中目标");
        return;
    }
    ImGui::TextUnformatted(std::format("正在为玩家：{}（{}）的物品类型 {} 进行修改",
        targetUID, targetPlayerName, targetWeaponName).c_str());

    ImGui::TextUnformatted("提示：所有滑块可以按Tab进行输入");
    ImGui::SliderInt("皮肤索引", &paintKit, 0, 10000);
    ImGui::SliderInt("模板种子", &seed, 0, 10000);
    ImGui::SliderFloat("磨损度", &wear, 0.0f, 1.0f);

    if (ImGui::Button("确认修改")) {
        auto [mPaintKit, p1] = MulNX::Message::Create<MulNX::NetExt>("SkinChange/PaintKit/Set"_hash);
        auto&& [steamID1, iPaintKit] = mPaintKit.Access<Steam64UID, int>();
        p1->str1 = targetWeaponName;
        steamID1 = targetUID;
        iPaintKit = paintKit;
        this->PublishAsync(std::move(mPaintKit));

        auto [mSeed, p2] = MulNX::Message::Create<MulNX::NetExt>("SkinChange/Seed/Set"_hash);
        auto&& [steamID2, iSeed] = mSeed.Access<Steam64UID, int>();
        p2->str1 = targetWeaponName;
        steamID2 = targetUID;
        iSeed = seed;
        this->PublishAsync(std::move(mSeed));

        auto [mWear, p3] = MulNX::Message::Create<MulNX::NetExt>("SkinChange/Wear/Set"_hash);
        auto&& [steamID3, fWear] = mWear.Access<Steam64UID, float>();
        p3->str1 = targetWeaponName;
        steamID3 = targetUID;
        fWear = wear;
        this->PublishAsync(std::move(mWear));

        this->PublishAsync("SkinChange/Refresh"_hash);
    }
    ImGui::SameLine();
    if (ImGui::Button("放弃修改")) {
        targetUID = 0;
    }
    ImGui::TextUnformatted("提示：少部分修改可能需要刷新回合甚至重入Demo");
}
