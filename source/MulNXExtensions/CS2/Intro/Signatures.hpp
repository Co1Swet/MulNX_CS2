#pragma once

#include <MulNXUtils/WinExt/WinExt.hpp>

namespace MulNX {
    namespace CS2 {
        namespace Signatures {
            inline const static MulNX::Memory::Pattern CallIsPlayingDemo("48 8b 0d ?? ?? ?? ?? 48 8b 01 ff 90 50 01 00 00 0f 57 ff 84 c0 74 63 ba ff ff ff ff");
            namespace Render {
                inline const static MulNX::Memory::Pattern CSHashString("FF D0 8B F0 48 85 FF 74 2A");
            }
            namespace Utils {
                inline const static MulNX::Memory::Pattern CSHashString("48 8B 58 ?? 0F 29 B4 24 C0 20 00 00 E8 ?? ?? ?? ??");
                inline const static MulNX::Memory::Pattern Pos_Call_CInputService_ProcessCommands("E8 ?? ?? ?? ?? 4C 8B BC 24 ?? ?? 00 00 45 84 ED");
                inline const static MulNX::Memory::Pattern RegenerateWeaponSkins("48 83 EC ?? E8 ?? ?? ?? ?? 48 85 C0 0F 84 ?? ?? ?? ?? 48 8B 10");
                inline const static MulNX::Memory::Pattern SetGlowColor("40 53 48 83 EC 20 48 8B D9 48 83 C1 40 39 11 ?? ?? 89 11 ?? ?? ?? ?? ?? 48 8B 4B 18 48 85 C9 ?? ?? 48 83");
                inline const static MulNX::Memory::Pattern GetDecoratedPlayerName("44 89 44 24 18 48 89 54 24 10 55 53 56 57 41 54 41 55 41 56 41 57 48 8d ac 24 28 f5 ff ff");
            }
            namespace Projectile {
                inline const static MulNX::Memory::Pattern SetSmokeProps("40 53 48 83 EC ?? 8B 91 ?? ?? ?? ?? 48 8B D9 85 D2 75");
                inline const static MulNX::Memory::Pattern Func_BaseCSGrenadeProjectile_DrawStuff("40 55 53 48 8D 6C 24 ?? 48 81 EC ?? ?? ?? ?? 80 B9");
            }
            namespace Hud {
                inline const static MulNX::Memory::Pattern HandlePlayerDeath("48 89 54 24 10 48 89 4C 24 08 55 53 56 57 41 54 48 8D AC 24 10 E0 FF FF B8 F0 ?? ?? ?? E8 ?? ?? ?? ?? 48 2B");
                inline const static MulNX::Memory::Pattern CLayoutFile_LoadFromFile("48 89 5C 24 08 55 56 57 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC 60 48 8D 05 ?? ?? ?? ?? 48 C7 45 D0 F4 03 00 00 48");
                inline const static MulNX::Memory::Pattern ifShowSpeaker("48 63 C3 48 8D 0D ?? ?? ?? ?? C6 84 08 ?? ?? ?? ?? 01 48 8B 0D ?? ?? ?? ?? 48 8B 01 FF 90 ?? ?? ?? ?? 84 C0 0F 85");
                inline const static MulNX::Memory::Pattern PosTeamID_CmpForHide("0F 5B FF 0F 2F FE 0F 82 ?? ?? ?? 00 F3 0F 10 44 24");
                inline const static MulNX::Memory::Pattern PosTeamID_xxIt("41 BC FF FF 00 00 48 8B ?? ?? 33 DB 48 8B FB 66 44 ?? ?? ?? ?? 0F 84 ?? ?? ?? 00");
                inline const static MulNX::Memory::Pattern PosTeamCounterWriteHP("8B 43 04 89 47 0C 8B 43 08");
            }
            namespace Flash {
                inline const static MulNX::Memory::Pattern PosCallCmpDrawFlashUpHUD("48 8B F2 48 8B E9 E8 ?? ?? ?? ?? 84 C0 0F 85");
                inline const static MulNX::Memory::Pattern PosCallCmpDrawFlashDownHUD("44 0F 28 94 24 ?? ?? ?? ?? 48 8B BC 24 ?? ?? ?? ?? 48 8B B4 24 ?? ?? ?? ?? 84 C0");
            }
            namespace Spot {
                inline const static MulNX::Memory::Pattern Pos_CmpToSetShow("38 5C 24 ?? 0F 84 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? F3 41 0F 10 8E ?? ?? ?? ?? F3 0F 10 41");// 从cvar定位
                inline const static MulNX::Memory::Pattern Pos_WriteMaybeEnumToChangeRadarPlayerDraw("48 8B 6C 24 ?? 41 39 9E ?? ?? ?? ?? 74 ?? 33 D2");// 交叉引用
                inline const static MulNX::Memory::Pattern Func_FinallyUpdatePlayerState("44 8B F2 41 81 E6 FF FF FF ?? 8B D8 41 33 DE");// 交叉引用
                inline const static MulNX::Memory::Pattern Pos_WriteBombState("41 FF D0 8B 0D ?? ?? ?? ?? 65 48 8B 04 25 ?? ?? ?? ?? 41 8B D4");// 交叉引用

                inline const static MulNX::Memory::Pattern Pos_CmpToSetColor("4C 89 6C 24 ?? 84 DB 0F 84");// 附近引用了 字符串： CCSGO_HudTeamCounter
                inline const static MulNX::Memory::Pattern Pos_CmpToSetTColor("E8 ?? ?? ?? ?? 41 3B C5 0F 85 ?? ?? ?? ?? F6 86");// 附近引用了 字符串： CCSGO_HudTeamCounter
                inline const static MulNX::Memory::Pattern Pos_CmpToSetCTColor("E8 ?? ?? ?? ?? 83 F8 03 75 ?? 8B D3");// 附近引用了 字符串： CCSGO_HudTeamCounter
            }
            // "particles/entity/spectator_utility_trail.vpcf", m_nSnapshotTrajectoryEffectIndex
            namespace Particle {
                inline const static MulNX::Memory::Pattern Func_ParticleManager_Get("48 8B 05 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC 48 89 5C 24 10 57");
                inline const static MulNX::Memory::Pattern Func_ParticleManager_CreateParticle("4C 8B DC 53 48 81 EC ?? ?? ?? ?? F2 0F 10 05");
                inline const static MulNX::Memory::Pattern Func_ParticleManager_UpdateParticle("48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC ?? F3 0F 10 1D ?? ?? ?? ?? 41 8B F8 8B DA 4C 8D 05");
            }
            namespace Sky {
                inline const static MulNX::Memory::Pattern Pos_C_EnvSky_VF10_Call_ForceUpdateSkybox("33 DB 48 8D 05 ?? ?? ?? ?? 48 8B CF 48 89 44 24 ??");
            }
            namespace Sound {
                inline const static MulNX::Memory::Pattern EmitHurtFeedbackSound("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 81 EC ?? ?? ?? ?? 49 8B E8");
            }
        }
    }
}