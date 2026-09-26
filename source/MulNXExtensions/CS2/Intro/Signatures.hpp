#pragma once
#include <MulNXUtils/WinExt/WinExt.hpp>

namespace MulNX {
    namespace CS2 {
        namespace Signatures {
            inline const static MulNX::Memory::Pattern Pos_ClientModeCSNormal_MaybeWriteView_CallIsPlayingDemo("48 8B 0D ?? ?? ?? ?? 48 8B 01 FF 90 ?? ?? ?? ?? 0F 57 FF 84 C0 74 5E");
            namespace Render {
                inline const static MulNX::Memory::Pattern Pos_Call_Present("FF 50 40 80 7C 24 ?? 00 44 8B E0 74 10");
            }
            namespace Utils {
                inline const static MulNX::Memory::Pattern CSHashString("48 83 EC 28 45 8B D0 4C 8B C9 48 83 FA 04 0F 82 ?? ?? ?? ?? 0F B6 09 48 89 5C 24 20 8D 41 BF 3C 19 77 03 80 C1 20");
                inline const static MulNX::Memory::Pattern CUtlStringRef__Assign("48 89 5C 24 10 57 48 83 EC 20 48 8B 41 18 49 8B F8 48 8B D9 4C 3B C0 77 34");
                inline const static MulNX::Memory::Pattern Pos_Call_CInputService_ProcessCommands("E8 ?? ?? ?? ?? 4C 8B BC 24 ?? ?? 00 00 45 84 ED");
                inline const static MulNX::Memory::Pattern Pos_CGameEventManager_FireEvents_AcquiredLock("C7 44 24 ?? 00 00 00 00 33 D2 48 8B CE");
                inline const static MulNX::Memory::Pattern RegenerateWeaponSkins("48 83 EC ?? E8 ?? ?? ?? ?? 48 85 C0 0F 84 ?? ?? ?? ?? 48 8B 10");
                inline const static MulNX::Memory::Pattern SetGlowColor("40 53 48 83 EC 20 48 8B D9 48 83 C1 40 39 11 74 02 89 11 E8 ?? ?? ?? ?? 48 8B 4B 18 48 85 C9");
                inline const static MulNX::Memory::Pattern GetDecoratedPlayerName("48 8B 01 FF 50 10 48 8B ?? 48 85 C0 0F 84 ?? ?? ?? ?? 80 38 00");
            }
            namespace Projectile {
                inline const static MulNX::Memory::Pattern SetSmokeProps("40 53 48 83 EC ?? 8B 91 ?? ?? ?? ?? 48 8B D9 85 D2 75");
                inline const static MulNX::Memory::Pattern Func_BaseCSGrenadeProjectile_DrawStuff("40 55 53 48 8D 6C 24 ?? 48 81 EC ?? ?? ?? ?? 80 B9");
            }
            namespace Hud {
                inline const static MulNX::Memory::Pattern HandlePlayerDeath("4C 8B F2 41 B8 FA DA 03 3E BA 02 00 00 00 48 8B 58 50 E8 ?? ?? ?? ??");
                inline const static MulNX::Memory::Pattern CLayoutFile_LoadFromFile("48 89 5C 24 08 55 56 57 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC 60 48 8D 05 ?? ?? ?? ?? 48 C7 45 D0 ?? ?? 00 00 48");
                
                inline const static MulNX::Memory::Pattern PosTeamID_CmpForHide("66 44 0F 6E D1 45 0F 5B D2 44 0F 2F D0 0F 82 ?? ?? ?? 00");
                inline const static MulNX::Memory::Pattern PosTeamID_xxIt("48 8B 75 ?? 33 FF 41 BC FF FF 00 00");

                namespace TeamCounter {
                    inline const static MulNX::Memory::Pattern Func_FillPlayerSlotCache("41 57 F3 0F 10 8C 24 C8 00 00 00");
                    inline const static MulNX::Memory::Pattern Pos_UpdatePanoramaSpecTargetVisible("48 8B 48 08 48 8B 01 FF 90 ?? ?? 00 00 B9 68 00 00 00 41 8B 04 0F 39");
                    inline const static MulNX::Memory::Pattern Pos_UpdatePanoramaNameVisible("4C 8B 8A ?? ?? 00 00 0F B7 15 ?? ?? ?? ?? 41 FF D1 48 8B 13");
                    inline const static MulNX::Memory::Pattern Pos_UpdatePanoramaNameVisible2("FF 90 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 65 48 8B 04 25 ?? ?? ?? ?? C6 83");
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
                inline const static MulNX::Memory::Pattern PosCallCmpDrawFlashDownHUD("84 C0 74 4C 8B 85 ?? ?? ?? ?? 49 8D 8D ?? ?? ?? ?? 0F 10 85");
            }
            namespace Spot {
                inline const static MulNX::Memory::Pattern Pos_CallGetPawnMaybeSetAllHUD("0F B6 9F ?? ?? ?? ?? 48 8B F0 41 BD 80 00 00 00 84 DB");

                inline const static MulNX::Memory::Pattern Pos_CmpToSetShow("38 5C 24 ?? 0F 84 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? F3 41 0F 10 8E ?? ?? ?? ?? F3 0F 10 41");// 从cvar定位
                inline const static MulNX::Memory::Pattern Pos_WriteMaybeEnumToChangeRadarPlayerDraw("48 8B 6C 24 ?? 41 39 9E ?? ?? ?? ?? 74 ?? 33 D2");// 交叉引用
                inline const static MulNX::Memory::Pattern Func_FinallyUpdatePlayerState("44 8B F2 41 81 E6 FF FF FF ?? 8B D8 41 33 DE");// 交叉引用
                inline const static MulNX::Memory::Pattern Pos_WriteBombState("41 FF D0 65 48 8B 04 25 ?? ?? ?? ?? BB 68 00 00 00");// 交叉引用

                inline const static MulNX::Memory::Pattern Pos_CmpToSetColor("4C 89 6C 24 ?? 84 DB 0F 84");// 附近引用了 字符串： CCSGO_HudTeamCounter
                inline const static MulNX::Memory::Pattern Pos_CmpToSetTColor("E8 ?? ?? ?? ?? 41 3B C5 0F 85 ?? ?? ?? ?? F6 86");// 附近引用了 字符串： CCSGO_HudTeamCounter
                inline const static MulNX::Memory::Pattern Pos_CmpToSetCTColor("E8 ?? ?? ?? ?? 83 F8 03 75 ?? 8B D3");// 附近引用了 字符串： CCSGO_HudTeamCounter
            }
            // "particles/entity/spectator_utility_trail.vpcf", m_nSnapshotTrajectoryEffectIndex
            namespace Particle {
                inline const static MulNX::Memory::Pattern Func_ParticleManager_Get("48 8B 05 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC 48 89 5C 24 10 57");
                inline const static MulNX::Memory::Pattern Func_ParticleManager_CreateParticle("4C 8B DC 53 48 81 EC ?? ?? ?? ?? F2 0F 10 05");
                inline const static MulNX::Memory::Pattern Func_ParticleManager_UpdateParticle("48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC ?? F3 0F 10 1D ?? ?? ?? ?? 41 8B F8 8B DA 4C 8D 05");
                inline const static MulNX::Memory::Pattern Func_BindTrail("40 56 48 83 EC 20 41 8B F0 49 8B C1");
            }
            namespace Sky {
                inline const static MulNX::Memory::Pattern Pos_C_EnvSky_VF10_Call_ForceUpdateSkybox("33 DB 48 8D 05 ?? ?? ?? ?? 48 8B CF 48 89 44 24 ??");
            }
            namespace Sound {
                inline const static MulNX::Memory::Pattern ifShowSpeaker("84 C0 0F 85 ?? ?? 00 00 48 8B 0D ?? ?? ?? ?? 48 8B 01 FF 90 ?? ?? ?? ?? 85 C0");
                inline const static MulNX::Memory::Pattern GetVoiceStatus("48 8B 05 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC 48 8D 05");
                inline const static MulNX::Memory::Pattern UpdateSpeakerStatus("48 89 5C 24 08 55 56 57 48 83 EC 30 48 8B 05");

                inline const static MulNX::Memory::Pattern EmitHurtFeedbackSound("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 81 EC ?? ?? ?? ?? 49 8B E8");
                inline const static MulNX::Memory::Pattern Pos_CallGetPawnUpdateCirclePos("48 85 C0 0F 84 ?? ?? 00 00 48 89 6C 24 ?? 48 8D 54 24 ?? 48 89 74 24 ?? 48 8B C8");
                inline const static MulNX::Memory::Pattern Pos_CallGetPawnMaybeLocalPawnsAsyncSoundEnque("48 3B D8 75 39 0F 28 DE");
                inline const static MulNX::Memory::Pattern Pos_CallGetPawnMaybeOtherAsyncSoundEnque("48 3B F8 0F 85 ?? ?? ?? ?? 0F 28 DE 40");

                inline const static MulNX::Memory::Pattern Func_ProcessVoiceBan("48 63 F8 85 F6 75");
            }
            namespace GSI {
                inline const static MulNX::Memory::Pattern Pos_GSI_ID_ForSpecTarget_call_GetOBingPawn("48 8B F0 48 85 C0 0F 84 ?? ?? 00 00 8B 90 ?? ?? 00 00");
            }
            namespace MapRemap {
                namespace Protobuf {
                    inline const static MulNX::Memory::Pattern Pos_DemoFileHeader_PraseString("48 8B D8 48 85 C0 0F 85 1F FF FF FF E9 4A 03 00 00");
                    inline const static MulNX::Memory::Pattern Pos_CNETMsg_SpawnGroup_Load_PraseString("48 8B D8 48 85 C0 0F 85 1C FF FF FF E9 58 05 00 00");
                    inline const static MulNX::Memory::Pattern Pos_CSVCMsg_ServerInfo_PraseString("48 8B D8 48 85 C0 0F 85 14 FC FF FF E9 16 01 00 00");
                    inline const static MulNX::Memory::Pattern Pos_CSVCMsg_ClearAllStringTables_PraseString("48 8B D8 48 85 C0 0F 85 C0 FE FF FF EB 60");
                    inline const static MulNX::Memory::Pattern Pos_CSVCMsg_GameSessionConfiguration_PraseString("48 8B D8 48 85 C0 0F 85 ?? ?? ?? ?? E9 ?? ?? ?? ?? 40 80 FF 52 0F 85 ?? ?? ?? ?? 83 4E");
                }
                inline const static MulNX::Memory::Pattern Pos_MapKVReaded("89 5C ?? ?? A9 FF FF FF 7F 76 1C C1 E8 1F 84 C0 75 15 48 8B 05 ?? ?? ?? ?? 48 8B 54 ?? ?? 48 8B 08 48 8B 01 FF 50 ?? 48 85 ED 48 8B 6C");

                inline const static MulNX::Memory::Pattern Pos_Log_Failedloading("8B 0D ?? ?? ?? ?? 4C 8D 0D ?? ?? ?? ?? 41 B8 FF 00 00 FF 48 89 5C ?? ?? BA 03 00 00 00 48 89 44 ?? ?? FF 15");
                inline const static MulNX::Memory::Pattern Pos_Manifest_AddFullPath("FF 15 ?? ?? ?? ?? 0F B7 85 ?? ?? ?? ?? B9 FF FF 00 00 66 3B C1 74 32 4C 8B B5");

                inline const static MulNX::Memory::Pattern Func_RequestResourceByHash("48 89 5C 24 10 48 89 74 24 18 57 48 83 EC 20 49 8B F8 48 8B DA");
                inline const static MulNX::Memory::Pattern Func_FindResourceByHash("48 89 5C 24 18 48 89 6C 24 20 56 57 41 56 48 83 EC 20");
            }
        }
    }
}