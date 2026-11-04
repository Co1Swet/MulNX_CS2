#pragma once
#include <Game/BaseType.hpp>
#include <MulNX/Config/Config.hpp>
#include <MulNXThirdParty/cs2_dumpers.hpp>

namespace CS2 {
#pragma pack(push, 1)
    class C_CSGameRules {
    public:
        char pad_0x0000[0x40]; // 0x0000
        bool m_bFreezePeriod; // 0x0040
        bool m_bWarmupPeriod; // 0x0041
        char pad_0x0042[0x02]; // 0x0042
        GameTime_t m_fWarmupPeriodEnd; // 0x0044
        GameTime_t m_fWarmupPeriodStart; // 0x0048
        bool m_bTerroristTimeOutActive; // 0x004C
        bool m_bCTTimeOutActive; // 0x004D
        char pad_0x004E[0x02]; // 0x004E
        float m_flTerroristTimeOutRemaining; // 0x0050
        float m_flCTTimeOutRemaining; // 0x0054
        int32_t m_nTerroristTimeOuts; // 0x0058
        int32_t m_nCTTimeOuts; // 0x005C
        bool m_bTechnicalTimeOut; // 0x0060
        bool m_bMatchWaitingForResume; // 0x0061
        char pad_0x0062[0x02]; // 0x0062
        int32_t m_iFreezeTime; // 0x0064
        int32_t m_iRoundTime; // 0x0068
        float m_fMatchStartTime; // 0x006C
        GameTime_t m_fRoundStartTime; // 0x0070
        GameTime_t m_flRestartRoundTime; // 0x0074
        bool m_bGameRestart; // 0x0078
        char pad_0x0079[0x03]; // 0x0079
        float m_flGameStartTime; // 0x007C
        float m_timeUntilNextPhaseStarts; // 0x0080
        int32_t m_gamePhase; // 0x0084
        int32_t m_totalRoundsPlayed; // 0x0088
        int32_t m_nRoundsPlayedThisPhase; // 0x008C
        int32_t m_nOvertimePlaying; // 0x0090
        int32_t m_iHostagesRemaining; // 0x0094
        bool m_bAnyHostageReached; // 0x0098
        bool m_bMapHasBombTarget; // 0x0099
        bool m_bMapHasRescueZone; // 0x009A
        bool m_bMapHasBuyZone; // 0x009B
        bool m_bIsQueuedMatchmaking; // 0x009C
        char pad_0x009D[0x03]; // 0x009D
        int32_t m_nQueuedMatchmakingMode; // 0x00A0
        bool m_bIsValveDS; // 0x00A4
        bool m_bLogoMap; // 0x00A5
        bool m_bPlayAllStepSoundsOnServer; // 0x00A6
        char pad_0x00A7[0x01]; // 0x00A7
        int32_t m_iSpectatorSlotCount; // 0x00A8
        int32_t m_MatchDevice; // 0x00AC
        bool m_bHasMatchStarted; // 0x00B0
        char pad_0x00B1[0x03]; // 0x00B1
        int32_t m_nNextMapInMapgroup; // 0x00B4
        char m_szTournamentEventName[512]; // 0x00B8
        char m_szTournamentEventStage[512]; // 0x02B8
        char m_szMatchStatTxt[512]; // 0x04B8
        char m_szTournamentPredictionsTxt[512]; // 0x06B8
        int32_t m_nTournamentPredictionsPct; // 0x08B8
        GameTime_t m_flCMMItemDropRevealStartTime; // 0x08BC
        GameTime_t m_flCMMItemDropRevealEndTime; // 0x08C0
        bool m_bIsDroppingItems; // 0x08C4
        bool m_bIsQuestEligible; // 0x08C5
        bool m_bIsHltvActive; // 0x08C6
        bool m_bBombPlanted; // 0x08C7
        uint16_t m_arrProhibitedItemIndices[100]; // 0x08C8
        uint32_t m_arrTournamentActiveCasterAccounts[4]; // 0x0990
        int32_t m_numBestOfMaps; // 0x09A0
        int32_t m_nHalloweenMaskListSeed; // 0x09A4
        bool m_bBombDropped; // 0x09A8
        char pad_0x09A9[0x03]; // 0x09A9
        int32_t m_iRoundWinStatus; // 0x09AC
        int32_t m_eRoundWinReason; // 0x09B0
        bool m_bTCantBuy; // 0x09B4
        bool m_bCTCantBuy; // 0x09B5
        char pad_0x09B6[0x02]; // 0x09B6
        int32_t m_iMatchStats_RoundResults[30]; // 0x09B8
        int32_t m_iMatchStats_PlayersAlive_CT[30]; // 0x0A30
        int32_t m_iMatchStats_PlayersAlive_T[30]; // 0x0AA8
        float m_TeamRespawnWaveTimes[32]; // 0x0B20
        int32_t m_nActiveMutators; // 0x0BA0 (新版新增字段)
        GameTime_t m_flNextRespawnWave[32]; // 0x0BA4
        char pad_0x0C24[0x0C]; // 0x0C24 VectorWS m_vMinimapMins
        char pad_0x0C30[0x0C]; // 0x0C30 VectorWS m_vMinimapMaxs
        float m_MinimapVerticalSectionHeights[8]; // 0x0C3C
        char pad_0x0C5C[0x04]; // 0x0C5C 补齐内存对齐
        uint64_t m_ullLocalMatchID; // 0x0C60
        int32_t m_nEndMatchMapGroupVoteTypes[10]; // 0x0C68
        int32_t m_nEndMatchMapGroupVoteOptions[10]; // 0x0C90
        int32_t m_nEndMatchMapVoteWinner; // 0x0CB8
        int32_t m_iNumConsecutiveCTLoses; // 0x0CBC
        int32_t m_iNumConsecutiveTerroristLoses; // 0x0CC0
        char pad_0x0CC4[0xBC]; // 0x0CC4
        int32_t m_nMatchAbortedEarlyReason; // 0x0D80
        bool m_bHasTriggeredRoundStartMusic; // 0x0D84
        bool m_bSwitchingTeamsAtRoundReset; // 0x0D85
        char pad_0x0D86[0x1A]; // 0x0D86
        char pad_0x0DA0[0x08]; // 0x0DA0 CCSGameModeRules* m_pGameModeRules
        char pad_0x0DA8[0x158]; // 0x0DA8 C_RetakeGameRules m_RetakeRules
        uint8_t m_nMatchEndCount; // 0x0F00
        char pad_0x0F01[0x03]; // 0x0F01
        int32_t m_nTTeamIntroVariant; // 0x0F04
        int32_t m_nCTTeamIntroVariant; // 0x0F08
        bool m_bTeamIntroPeriod; // 0x0F0C
        char pad_0x0F0D[0x03]; // 0x0F0D
        int32_t m_iRoundEndWinnerTeam; // 0x0F10
        int32_t m_eRoundEndReason; // 0x0F14
        bool m_bRoundEndShowTimerDefend; // 0x0F18
        char pad_0x0F19[0x03]; // 0x0F19
        int32_t m_iRoundEndTimerTime; // 0x0F1C
        char pad_0x0F20[0x08]; // 0x0F20 CUtlString m_sRoundEndFunFactToken
        char pad_0x0F28[0x04]; // 0x0F28 CPlayerSlot m_iRoundEndFunFactPlayerSlot
        int32_t m_iRoundEndFunFactData1; // 0x0F2C
        int32_t m_iRoundEndFunFactData2; // 0x0F30
        int32_t m_iRoundEndFunFactData3; // 0x0F34
        char pad_0x0F38[0x08]; // 0x0F38 CUtlString m_sRoundEndMessage
        int32_t m_iRoundEndPlayerCount; // 0x0F40
        bool m_bRoundEndNoMusic; // 0x0F44
        char pad_0x0F45[0x03]; // 0x0F45
        int32_t m_iRoundEndLegacy; // 0x0F48
        uint8_t m_nRoundEndCount; // 0x0F4C
        char pad_0x0F4D[0x03]; // 0x0F4D
        int32_t m_iRoundStartRoundNumber; // 0x0F50
        uint8_t m_nRoundStartCount; // 0x0F54
        char pad_0x0F55[0x400B]; // 0x0F55
        double m_flLastPerfSampleTime; // 0x4F60

        inline bool IsGameInactive() {
            try {
                auto bIsFreeze = MulNX::MRead(&this->m_bFreezePeriod);
                auto flRestartRoundTime = MulNX::MRead(&this->m_flRestartRoundTime);
                return bIsFreeze || flRestartRoundTime;
            }
            catch (...) {
                return true;
            }
        }
    };
#pragma pack(pop)
}
// 验证开头的 bool
static_assert(
    offsetof(CS2::C_CSGameRules, m_bFreezePeriod) ==
    cs2_dumper::schemas::client_dll::C_CSGameRules::m_bFreezePeriod,
    "C_CSGameRules::m_bFreezePeriod offset mismatch!"
    );
// 验证新插入的 m_nActiveMutators
static_assert(
    offsetof(CS2::C_CSGameRules, m_nActiveMutators) ==
    cs2_dumper::schemas::client_dll::C_CSGameRules::m_nActiveMutators,
    "C_CSGameRules::m_nActiveMutators offset mismatch!"
    );
// 验证跨越对齐边界的 uint64
static_assert(
    offsetof(CS2::C_CSGameRules, m_ullLocalMatchID) ==
    cs2_dumper::schemas::client_dll::C_CSGameRules::m_ullLocalMatchID,
    "C_CSGameRules::m_ullLocalMatchID offset mismatch!"
    );
// 验证用到的 m_nRoundStartCount
static_assert(
    offsetof(CS2::C_CSGameRules, m_nRoundStartCount) ==
    cs2_dumper::schemas::client_dll::C_CSGameRules::m_nRoundStartCount,
    "C_CSGameRules::m_nRoundStartCount offset mismatch!"
    );
// 验证结尾的 double
static_assert(
    offsetof(CS2::C_CSGameRules, m_flLastPerfSampleTime) ==
    cs2_dumper::schemas::client_dll::C_CSGameRules::m_flLastPerfSampleTime,
    "C_CSGameRules::m_flLastPerfSampleTime offset mismatch!"
    );