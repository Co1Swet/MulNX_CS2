#include "BackgroundEntityScan.hpp"
#include "EntityIterationMixin.hpp"

bool BackgroundEntityScan::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](MulNX::Message& msg) {
        this->SendTask("Main", "CSControl", [this]() {
            try {
                this->Main();
            }
            catch (MulNX::Exception& e) {
                this->LogError(e);
            }
            return true;
            });
        });

    return true;
}

void BackgroundEntityScan::Main() {
    auto pGameRules = this->CS2->client.dwGameRules();
    if (!pGameRules) return;

    static int32_t OldRoundStartCount = 0;
    auto currentStartCount = MulNX::MRead(pGameRules->m_nRoundStartCount());
    if (OldRoundStartCount != currentStartCount) {
        this->PublishAsync("Game/NewRound"_hash);
        OldRoundStartCount = currentStartCount;
    }
    // 玩家控制器，地图上从1到10
    int playerNum = 0;
    for (const auto& mod : this->ParticipateItCSModules) {
        mod->OnItBegin();
    }
    for (int i = 0; i < this->CS2->client.dwGameEntitySystem_highestEntityIndex(); ++i) {
        auto* entity = this->CS2->client.GetBaseEntity(i);
        if (!entity)continue;

        auto* controller = entity->As<CS2::CCSPlayerController>();
        auto hPawn = MulNX::MRead(controller->m_hPlayerPawn());
        auto* pawn = this->CS2->client.GetBaseEntityFromHandle(hPawn.GetIndexInEntityList())->As<CS2::C_CSPlayerPawn>();
        if (!pawn)continue;

        auto team = MulNX::MRead(pawn->iTeamNum());
        if (team != CS2::ui8TeamNum::T && team != CS2::ui8TeamNum::CT)continue;
        ++playerNum;

        for (const auto& mod : this->ParticipateItCSModules) {
            mod->OnItPlayer(i, controller, pawn);
        }

        if (playerNum <= 10) {
            std::unique_lock lock(this->smutex);
            auto& CS2EBEntity = this->CS2EBGameData.Players[playerNum];
            CS2EBEntity.Position = MulNX::MRead(pawn->vOldOrigin());
            CS2EBEntity.EyePosition = MulNX::MRead(pawn->vOldOrigin()) + MulNX::MRead(pawn->vecViewOffset());
            CS2EBEntity.Rotation = MulNX::MRead(pawn->angEyeAngles());
            CS2EBEntity.HP = MulNX::MRead(pawn->iHealth());
            CS2EBEntity.Team = static_cast<int>(team);
            CS2EBEntity.Alive = CS2EBEntity.HP;
            CS2EBEntity.IndexInMap = playerNum;
        }
    }
    for (const auto& mod : this->ParticipateItCSModules) {
        mod->OnItEnd();
    }
    return;
}

D_Player& BackgroundEntityScan::GetPlayerMsg(int Index) {
    return this->CS2EBGameData.Players[Index];
}