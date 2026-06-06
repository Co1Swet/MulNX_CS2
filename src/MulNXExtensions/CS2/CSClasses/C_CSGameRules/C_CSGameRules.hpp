#pragma once

#include <MulNX/Config/Config.hpp>
#include <MulNXThirdParty/All_cs2_dumper.hpp>

namespace CS2 {
    class C_CSGameRules {
    public:
        int32_t* m_iRoundStartRoundNumber() {
            return Schema<int32_t>(this, cs2_dumper::schemas::client_dll::C_CSGameRules::m_iRoundStartRoundNumber);
        }
        // 回合计数（uint8_t）
        uint8_t* m_nRoundStartCount() {
            return Schema<uint8_t>(this, cs2_dumper::schemas::client_dll::C_CSGameRules::m_nRoundStartCount);
        }

        // 游戏开始与匹配时间（GameTime_t）
        GameTime_t* m_flGameStartTime() {
            return Schema<GameTime_t>(this, cs2_dumper::schemas::client_dll::C_CSGameRules::m_flGameStartTime);
        }
        GameTime_t* m_fMatchStartTime() {
            return Schema<GameTime_t>(this, cs2_dumper::schemas::client_dll::C_CSGameRules::m_fMatchStartTime);
        }

        // 热身阶段时间（GameTime_t）
        GameTime_t* m_fWarmupPeriodStart() {
            return Schema<GameTime_t>(this, cs2_dumper::schemas::client_dll::C_CSGameRules::m_fWarmupPeriodStart);
        }
        GameTime_t* m_fWarmupPeriodEnd() {
            return Schema<GameTime_t>(this, cs2_dumper::schemas::client_dll::C_CSGameRules::m_fWarmupPeriodEnd);
        }

        // 回合时间（GameTime_t）
        GameTime_t* m_fRoundStartTime() {
            return Schema<GameTime_t>(this, cs2_dumper::schemas::client_dll::C_CSGameRules::m_fRoundStartTime);
        }
        GameTime_t* m_flRestartRoundTime() {
            return Schema<GameTime_t>(this, cs2_dumper::schemas::client_dll::C_CSGameRules::m_flRestartRoundTime);
        }

        // 阶段切换倒计时（GameTime_t，实际为剩余秒数）
        GameTime_t* m_timeUntilNextPhaseStarts() {
            return Schema<GameTime_t>(this, cs2_dumper::schemas::client_dll::C_CSGameRules::m_timeUntilNextPhaseStarts);
        }

        // 冻结时间 & 回合时长（int32 秒数）
        int32_t* m_iFreezeTime() {
            return Schema<int32_t>(this, cs2_dumper::schemas::client_dll::C_CSGameRules::m_iFreezeTime);
        }
        int32_t* m_iRoundTime() {
            return Schema<int32_t>(this, cs2_dumper::schemas::client_dll::C_CSGameRules::m_iRoundTime);
        }

        // 暂停剩余时间（float32 秒数）
        float* m_flTerroristTimeOutRemaining() {
            return Schema<float>(this, cs2_dumper::schemas::client_dll::C_CSGameRules::m_flTerroristTimeOutRemaining);
        }
        float* m_flCTTimeOutRemaining() {
            return Schema<float>(this, cs2_dumper::schemas::client_dll::C_CSGameRules::m_flCTTimeOutRemaining);
        }

        // 物品掉落展示时间段（GameTime_t）
        GameTime_t* m_flCMMItemDropRevealStartTime() {
            return Schema<GameTime_t>(this, cs2_dumper::schemas::client_dll::C_CSGameRules::m_flCMMItemDropRevealStartTime);
        }
        GameTime_t* m_flCMMItemDropRevealEndTime() {
            return Schema<GameTime_t>(this, cs2_dumper::schemas::client_dll::C_CSGameRules::m_flCMMItemDropRevealEndTime);
        }

        // 下一波复活时间数组（GameTime_t，返回首元素指针，按索引访问）
        GameTime_t* m_flNextRespawnWave() {
            return Schema<GameTime_t>(this, cs2_dumper::schemas::client_dll::C_CSGameRules::m_flNextRespawnWave);
        }

        // 队伍复活波次时间数组（float32 秒数，非 GameTime_t）
        float* m_TeamRespawnWaveTimes() {
            return Schema<float>(this, cs2_dumper::schemas::client_dll::C_CSGameRules::m_TeamRespawnWaveTimes);
        }

        // 上次性能采样时间（高精度 double）
        float* m_flLastPerfSampleTime() {
            return Schema<float>(this, cs2_dumper::schemas::client_dll::C_CSGameRules::m_flLastPerfSampleTime);
        }
    };
}