#pragma once
#include <MulNXUtils/WinExt/WinExt.hpp>

namespace MulNX {
    namespace CS2 {
        namespace Signatures {
            inline const static MulNX::Memory::Pattern Pos_CViewRenderer_VFuncsSubFunc_MaybeWriteView_CallIsPlayingDemo("48 8b 0d ?? ?? ?? ?? 48 8b 01 ff 90 50 01 00 00 0f 57 ff 84 c0 74 57 ba ff ff ff ff");
            namespace Render {
                inline const static MulNX::Memory::Pattern Pos_Call_Present("FF 50 40 80 7C 24 ?? 00 44 8B E0 74 10");
            }
            namespace Utils {
                inline const static MulNX::Memory::Pattern CSHashString("48 83 EC 28 45 8B D0 4C 8B C9 48 83 FA 04 0F 82 ?? ?? ?? ?? 0F B6 09 48 89 5C 24 20 8D 41 BF 3C 19 77 03 80 C1 20");
                inline const static MulNX::Memory::Pattern Pos_Call_CInputService_ProcessCommands("E8 ?? ?? ?? ?? 4C 8B BC 24 ?? ?? 00 00 45 84 ED");
                inline const static MulNX::Memory::Pattern Pos_CGameEventManager_FireEvents_AcquiredLock("C7 44 24 ?? 00 00 00 00 33 D2 49 8B CE");
                inline const static MulNX::Memory::Pattern RegenerateWeaponSkins("48 83 EC ?? E8 ?? ?? ?? ?? 48 85 C0 0F 84 ?? ?? ?? ?? 48 8B 10");
                inline const static MulNX::Memory::Pattern SetGlowColor("40 53 48 83 EC 20 48 8B D9 48 83 C1 40 38 11 75 1E 8B C2 C1 E8 08 38 41 01 75 14 8B C2 C1 E8 10 38 41 02 75 0A 8B C2 C1 E8 18 38 41 03 74 02 89 11 E8 ?? ?? ?? ?? 48 8B 4B 18 48 85 C9");
                inline const static MulNX::Memory::Pattern GetDecoratedPlayerName("48 8B 01 FF 50 10 4C 8B F0 48 85 C0");
            }
            namespace Projectile {
                inline const static MulNX::Memory::Pattern SetSmokeProps("40 53 48 83 EC ?? 8B 91 ?? ?? ?? ?? 48 8B D9 85 D2 75");
                inline const static MulNX::Memory::Pattern Func_BaseCSGrenadeProjectile_DrawStuff("40 55 53 48 8D 6C 24 ?? 48 81 EC ?? ?? ?? ?? 80 B9");
            }
            namespace Hud {
                inline const static MulNX::Memory::Pattern HandlePlayerDeath("4C 8B F2 41 B8 FA DA 03 3E BA 02 00 00 00 48 8B 58 50 E8 ?? ?? ?? ??");
                inline const static MulNX::Memory::Pattern CLayoutFile_LoadFromFile("48 89 5C 24 08 55 56 57 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC 60 48 8D 05 ?? ?? ?? ?? 48 C7 45 D0 F4 03 00 00 48");
                
                inline const static MulNX::Memory::Pattern PosTeamID_CmpForHide("0F 5B FF 0F 2F FE 0F 82 ?? ?? ?? 00 F3 0F 10 44 24");
                inline const static MulNX::Memory::Pattern PosTeamID_xxIt("48 8B 75 ?? 33 FF 41 BC FF FF 00 00");

                namespace TeamCounter {
                    inline const static MulNX::Memory::Pattern Func_FillPlayerSlotCache("41 57 F3 0F 10 8C 24 C8 00 00 00");
                    inline const static MulNX::Memory::Pattern Pos_UpdatePanoramaSpecTargetVisible("48 8B 48 08 48 8B 01 FF 90 ?? ?? 00 00 B9 68 00 00 00 41 8B 04 0F 39");
                    inline const static MulNX::Memory::Pattern Pos_UpdatePanoramaNameVisible("4C 8B 8A ?? ?? 00 00 0F B7 15 ?? ?? ?? ?? 41 FF D1 48 8B 13");
                    inline const static MulNX::Memory::Pattern Pos_UpdatePanoramaFullInfoVisible("FF 90 ?? ?? 00 00 E8 ?? ?? ?? ?? 48 8B 5E ?? 48 8B C8");
                }

                inline const static MulNX::Memory::Pattern Pos_CheckFor_HudSpecplayerRoot__visible("65 48 8B 04 25 58 00 00 00 8B 0D ?? ?? ?? ?? 41 BD 68 00 00 00 4C 8B 24 C8");
                inline const static MulNX::Memory::Pattern Pos_CheckFor_HUD__spectating_target("49 8B 86 ?? ?? 00 00 44 0F B6 C2");
            }
            namespace ViewModel {
                inline const static MulNX::Memory::Pattern Func_GetViewModelInfo("40 55 53 56 41 56 41 57 48 8B EC 48 83 EC 20 4D 8B F8 4C 8B F2 48 8B F1");
                inline const static MulNX::Memory::Pattern Func_GetIfHandLeftSide("40 53 48 83 EC ?? 80 B9 ?? ?? ?? ?? ?? 48 8B D9 0F 84 ?? ?? ?? ?? 48 8B 89 ?? ?? ?? ?? 48 85 C9 75");
            }
            namespace Flash {
                inline const static MulNX::Memory::Pattern PosCallCmpDrawFlashUpHUD("48 8B F2 48 8B E9 E8 ?? ?? ?? ?? 84 C0 0F 85");
                inline const static MulNX::Memory::Pattern PosCallCmpDrawFlashDownHUD("84 C0 74 4C 8B 85 B0 02 00 00 49 8D 8D 48 03 00 00");
            }
            namespace Spot {
                inline const static MulNX::Memory::Pattern Pos_CallGetPawnMaybeSetAllHUD("44 0F 29 9C 24 80 00 00 00 E8 ?? ?? ?? ?? 0F B6 BB 60 77 01 00 48 8B F0 41 BE 80 00 00 00");

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
                inline const static MulNX::Memory::Pattern ifShowSpeaker("84 C0 0F 85 ?? ?? 00 00 48 8B 0D ?? ?? ?? ?? 48 8B 01 FF 90 ?? ?? ?? ?? 85 C0");

                inline const static MulNX::Memory::Pattern EmitHurtFeedbackSound("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 81 EC ?? ?? ?? ?? 49 8B E8");
                inline const static MulNX::Memory::Pattern Pos_CallGetPawnUpdateCirclePos("48 85 C0 0F 84 ?? ?? 00 00 48 89 6C 24 ?? 48 8D 54 24 ?? 48 89 74 24 ?? 48 8B C8");
                inline const static MulNX::Memory::Pattern Pos_CallGetPawnMaybeLocalPawnsAsyncSoundEnque("48 3B C3 75 1A 0F 28 DE");
                inline const static MulNX::Memory::Pattern Pos_CallGetPawnMaybeOtherAsyncSoundEnque("48 3B C6 0F 85 ?? ?? ?? ?? 0F 28 DE");

                inline const static MulNX::Memory::Pattern Func_ProcessVoiceBan("48 63 F8 85 F6 75");
            }
            namespace GSI {
                inline const static MulNX::Memory::Pattern Pos_GSI_ID_ForSpecTarget_call_GetOBingPawn("48 8B F0 48 85 C0 0F 84 ?? ?? 00 00 8B 90 ?? ?? 00 00");
            }
        }
    }
}