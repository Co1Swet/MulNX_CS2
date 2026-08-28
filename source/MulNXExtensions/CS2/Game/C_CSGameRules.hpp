#pragma once
#include <Game/BaseType.hpp>
#include <MulNX/Config/Config.hpp>
#include <MulNXThirdParty/All_cs2_dumper.hpp>

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
        GameTime_t m_flNextRespawnWave[32]; // 0x0BA0
        char pad_0x0C20[0x0C]; // 0x0C20 VectorWS m_vMinimapMins
        char pad_0x0C2C[0x0C]; // 0x0C2C VectorWS m_vMinimapMaxs
        float m_MinimapVerticalSectionHeights[8]; // 0x0C38
        uint64_t m_ullLocalMatchID; // 0x0C58
        int32_t m_nEndMatchMapGroupVoteTypes[10]; // 0x0C60
        int32_t m_nEndMatchMapGroupVoteOptions[10]; // 0x0C88
        int32_t m_nEndMatchMapVoteWinner; // 0x0CB0
        int32_t m_iNumConsecutiveCTLoses; // 0x0CB4
        int32_t m_iNumConsecutiveTerroristLoses; // 0x0CB8
        char pad_0x0CBC[0xBC]; // 0x0CBC
        int32_t m_nMatchAbortedEarlyReason; // 0x0D78
        bool m_bHasTriggeredRoundStartMusic; // 0x0D7C
        bool m_bSwitchingTeamsAtRoundReset; // 0x0D7D
        char pad_0x0D7E[0x1A]; // 0x0D7E
        char pad_0x0D98[0x08]; // 0x0D98 CCSGameModeRules* m_pGameModeRules
        char pad_0x0DA0[0x158]; // 0x0DA0 C_RetakeGameRules m_RetakeRules
        uint8_t m_nMatchEndCount; // 0x0EF8
        char pad_0x0EF9[0x03]; // 0x0EF9
        int32_t m_nTTeamIntroVariant; // 0x0EFC
        int32_t m_nCTTeamIntroVariant; // 0x0F00
        bool m_bTeamIntroPeriod; // 0x0F04
        char pad_0x0F05[0x03]; // 0x0F05
        int32_t m_iRoundEndWinnerTeam; // 0x0F08
        int32_t m_eRoundEndReason; // 0x0F0C
        bool m_bRoundEndShowTimerDefend; // 0x0F10
        char pad_0x0F11[0x03]; // 0x0F11
        int32_t m_iRoundEndTimerTime; // 0x0F14
        char pad_0x0F18[0x08]; // 0x0F18 CUtlString m_sRoundEndFunFactToken
        char pad_0x0F20[0x04]; // 0x0F20 CPlayerSlot m_iRoundEndFunFactPlayerSlot
        int32_t m_iRoundEndFunFactData1; // 0x0F24
        int32_t m_iRoundEndFunFactData2; // 0x0F28
        int32_t m_iRoundEndFunFactData3; // 0x0F2C
        char pad_0x0F30[0x08]; // 0x0F30 CUtlString m_sRoundEndMessage
        int32_t m_iRoundEndPlayerCount; // 0x0F38
        bool m_bRoundEndNoMusic; // 0x0F3C
        char pad_0x0F3D[0x03]; // 0x0F3D
        int32_t m_iRoundEndLegacy; // 0x0F40
        uint8_t m_nRoundEndCount; // 0x0F44
        char pad_0x0F45[0x03]; // 0x0F45
        int32_t m_iRoundStartRoundNumber; // 0x0F48
        uint8_t m_nRoundStartCount; // 0x0F4C
        char pad_0x0F4D[0x400B]; // 0x0F4D
        double m_flLastPerfSampleTime; // 0x4F58
    };
#pragma pack(pop)
}