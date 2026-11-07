#include "AutoRecordRule.hpp"

void AutoRecordRule::Window() {
    auto window = MulNX::UI::RAIIWindow("自动录制规则");
    if (!window || !window.ShouldDraw()) {
        return;
    }

    auto currentOptions = this->GetOptions();
    bool changed = false;

    if (ImGui::RadioButton("所有玩家", currentOptions.mode == Mode::AllPlayers)) {
        currentOptions.mode = Mode::AllPlayers;
        changed = true;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("单个玩家", currentOptions.mode == Mode::SinglePlayer)) {
        currentOptions.mode = Mode::SinglePlayer;
        changed = true;
    }

    if (currentOptions.mode == Mode::SinglePlayer) {
        ImU64 playerUid = currentOptions.player;
        if (ImGui::InputScalar("目标玩家 Steam64 UID", ImGuiDataType_U64,
            &playerUid, nullptr, nullptr, "%llu")) {
            currentOptions.player = playerUid;
            changed = true;
        }
    }

    if (ImGui::Checkbox("同时生成受害者视角", &currentOptions.includeVictimView)) {
        changed = true;
    }

    if (ImGui::Checkbox("提交后自动启动录制", &currentOptions.autoStart)) {
        changed = true;
    }

    ImGui::TextDisabled("规则请求会按批次提交；勾选后，提交完成时自动启动录制。");

    if (changed) {
        this->SetOptions(currentOptions);
    }
}

bool AutoRecordRule::Init() {
    this->UIRegisterCallback("UI.Demos", [this](auto&&...) {
        this->Window();
        });

    this->SendTask("Update", "DemoSys", [this]() {
        this->Update();
        return true;
        });

    return true;
}

AutoRecordRule::Options AutoRecordRule::GetOptions() const {
    std::shared_lock lock(this->smutex);
    return this->options;
}

void AutoRecordRule::SetOptions(const Options& newOptions) {
    std::unique_lock lock(this->smutex);
    this->options = newOptions;
}

AutoRecordRule::Plan AutoRecordRule::BuildPlan(const Demo::Info& info) const {
    const auto currentOptions = this->GetOptions();
    Plan plan{};
    plan.autoStart = currentOptions.autoStart;

    auto addPlayerRequests = [&](const Demo::Player& player) {
        for (const auto& [roundNumber, roundInfo] : player.roundInfo) {
            if (!roundInfo.killEvents.empty()) {
                auto [request, ext] = MulNX::Message::Create<MulNX::NetExt>(
                    "Demo/Record/Make/RoundKill"_hash);
                ext->str1 = info.kDir;
                ext->str2 = info.demoFileName;
                auto&& [uid, round] = request.Access<Steam64UID, size_t>();
                uid = player.steamId;
                round = static_cast<size_t>(roundNumber);
                plan.requests.push_back(std::move(request));
            }

            if (currentOptions.includeVictimView && roundInfo.Bekilled.has_value()) {
                auto [request, ext] = MulNX::Message::Create<MulNX::NetExt>(
                    "Demo/Record/Make/Death"_hash);
                ext->str1 = info.kDir;
                ext->str2 = info.demoFileName;
                auto&& [uid, round] = request.Access<Steam64UID, size_t>();
                uid = player.steamId;
                round = static_cast<size_t>(roundNumber);
                plan.requests.push_back(std::move(request));
            }
        }
    };

    if (currentOptions.mode == Mode::SinglePlayer) {
        const auto it = info.players.find(currentOptions.player);
        if (it != info.players.end()) {
            addPlayerRequests(it->second);
        }
        return plan;
    }

    for (const auto& entry : info.players) {
        addPlayerRequests(entry.second);
    }
    return plan;
}
