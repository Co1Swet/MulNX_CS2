#include "DemoJSONReader.hpp"
#include <fstream>
#include <nlohmann/json.hpp>

bool DemoJSONReader::Window() {
    std::shared_lock lock(this->smutex, std::defer_lock);
    if (this->smutex.try_lock_shared()) {
        lock = std::shared_lock(this->smutex, std::adopt_lock);
    }
    else {
        ImGui::Text("正在读取JSON文件，请稍候...");
        return true;
    }

    return true;
}

bool DemoJSONReader::Init() {
    this->dirData = this->Path()->PathGetForShared("Data");
    this->SubscribeAsync("Demo/JSON/Read");

    this->UIRegisterCallback("UI.Demos", [this](auto&&...) {return this->Window();});

    this->SendTask("Update", "DemoSys", [this]()->bool {
        this->Update();
        return true;
        });

    return true;
}

void DemoJSONReader::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Demo/JSON/Read"_hash: {
        this->OnJSONMsg(msg);
        break;
    }
    }
}

void DemoJSONReader::OnJSONMsg(MulNX::Message& msg) {
    auto r = this->ParseDemoPath(msg);
    if (!r) {
        auto&& [key, demoName, errorMsg] = r.error();
        this->PublishAsync(this->MakeDemoInfoErrorNotify(key, demoName));
        this->LogError(std::format("解析 demo 路径请求处理失败: {}", errorMsg));
        return;
    }
    auto&& [kDir, name, path] = *r;
    std::string filename = name + ".json";

    std::unique_lock lock(this->smutex);

    fs::path filePath = this->dirData / filename;
    if (!fs::exists(filePath)) {
        this->LogError(std::format("文件不存在: {}", filePath.string()));
        this->PublishAsync(this->MakeDemoInfoErrorNotify(kDir, name));
        return;
    }
    try {
        return this->ReadJSON(std::move(kDir), filePath);
    }
    catch (const std::exception& e) {
        this->LogError(std::format("读取 JSON 文件时发生错误: {}，错误：{}", filePath.string(), e.what()));  
    }
    catch (...) {
        this->LogError(std::format("读取 JSON 文件时发生未知错误: {}", filePath.string()));
    }
    this->PublishAsync(this->MakeDemoInfoErrorNotify(kDir, name));
}

void DemoJSONReader::ReadJSON(std::string&& kDir, const fs::path& filePath) {
    auto json = nlohmann::json::parse(std::ifstream(filePath.wstring()));
    this->LogInfo("成功读取 JSON 文件: " + filePath.string());
    Demo::Info info{};
    info.kDir = kDir;
    info.demoFileName = json["demoFileName"].get<std::string>();
    info.mapName = json["mapName"].get<std::string>();
    info.tickCount = json["tickCount"].get<int>();

    info.teamA.name = json["teamA"]["name"].get<std::string>();
    info.teamA.letter = json["teamA"]["letter"].get<std::string>();
    info.teamA.score = json["teamA"]["score"].get<int>();

    info.teamB.name = json["teamB"]["name"].get<std::string>();
    info.teamB.letter = json["teamB"]["letter"].get<std::string>();
    info.teamB.score = json["teamB"]["score"].get<int>();

    auto playersJson = json["players"];
    for (auto it = playersJson.begin(); it != playersJson.end(); ++it) {
        const auto& playerJson = it.value();
        Demo::Player player;
        player.steamId = playerJson["steamId"].get<Steam64UID>();
        if (player.steamId == 0)continue;
        player.crosshairShareCode = playerJson["crosshairShareCode"].get<std::string>();
        player.name = playerJson["name"].get<std::string>();
        auto team = playerJson["team"];
        auto letter = team["letter"].get<std::string>();
        player.isTeamA = (letter == info.teamA.letter);
        player.killCount = playerJson["killCount"].get<int>();
        player.deathCount = playerJson["deathCount"].get<int>();
        player.assistCount = playerJson["assistCount"].get<int>();

        info.players[player.steamId] = std::move(player);
    }

    auto killsJson = json["kills"];
    for (auto it = killsJson.begin(); it != killsJson.end(); ++it) {
        const auto& killJson = it.value();
        Demo::KillEvent killEvent{};
        killEvent.tick = killJson["tick"].get<int>();
        killEvent.killerSteamId = killJson["killerSteamId"].get<Steam64UID>();
        killEvent.assisterSteamId = killJson["assisterSteamId"].get<Steam64UID>();
        killEvent.victimSteamId = killJson["victimSteamId"].get<Steam64UID>();

        if (killEvent.killerSteamId == 0 || killEvent.victimSteamId == 0) {
            continue;
        }
        if (killEvent.killerSteamId == killEvent.victimSteamId) {
            continue;
        }

        killEvent.weaponName = killJson["weaponName"].get<std::string>();
        killEvent.roundNumber = killJson["roundNumber"].get<int>();
        info.players[killEvent.victimSteamId].roundInfo[killEvent.roundNumber].Bekilled = killEvent;
        info.players[killEvent.killerSteamId].roundInfo[killEvent.roundNumber].killEvents.push_back(std::move(killEvent));
    }

    auto roundsJson = json["rounds"];
    for (auto it = roundsJson.begin();it != roundsJson.end();++it) {
        const auto& roundJson = it.value();
        Demo::Round round{};
        round.duration = roundJson["duration"].get<int>();
        round.endOfficiallyTick = roundJson["endOfficiallyTick"].get<int>();
        round.endTick = roundJson["endTick"].get<int>();
        round.freezeTimeEndTick = roundJson["freezeTimeEndTick"].get<int>();
        round.number = roundJson["number"].get<int>();
        round.startTick = roundJson["startTick"].get<int>();

        info.rounds.push_back(std::move(round));
    }

    auto [msg, rp] = MulNX::Message::Create<Demo::Info>("Demo/Info/Load"_hash, std::move(info));
    this->PublishAsync(std::move(msg));
    this->PublishAsync("Demo/Refresh"_hash);
}