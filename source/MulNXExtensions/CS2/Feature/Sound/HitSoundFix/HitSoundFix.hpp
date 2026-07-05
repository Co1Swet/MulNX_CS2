#pragma once
#include <Intro/CSModuleBase.hpp>
#include <Intro/HookGameEvents/HookGameEvents.hpp>

class HitSoundFix final :public CSModuleBase {
    using EmitHurtFeedbackSound_t = void(*)(CS2::C_CSPlayerPawn* pSourcePawn, CS2::C_CSPlayerPawn* pListenerPawn, const char* soundName);
    EmitHurtFeedbackSound_t EmitHurtFeedbackSound = nullptr;
    void HandleOnPlayerHurt(CS2::CGameEvent* event);
    bool Init()override;
};