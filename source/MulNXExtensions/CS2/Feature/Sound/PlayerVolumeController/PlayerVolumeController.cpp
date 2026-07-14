#include "PlayerVolumeController.hpp"

bool PlayerVolumeController::Init() {
    this->LogWarning("此模块拒绝工作，因为它只在对局能工作");

    this->SubscribeSync("Hook/LoadLibraryExW/soundsystem.dll", [this](auto) {
        this->soundsystem = MulNX::Memory::DllModule(L"soundsystem.dll");
        // this->hkSoundSystem001_SetPlayerVoiceVolume = this->CreateHook(
        //     "SoundSystem001_SetPlayerVoiceVolume",
        //     (uint8_t*)(this->soundsystem.GetBaseAddress() + 0x062750),
        //     [this](MulNX::Hook* hk, RegContext* ctx) {
        //         Steam64UID uid = ctx->rdx;
        //         return MulNX::Hook::Then::Continue;
        //     }).value();
        // this->hkSoundSystem001_SetPlayerVoiceVolume->Attach();

        // this->hkSoundSystem001_GetPlayerVoiceVolume = this->CreateHook(
        //     "SoundSystem001_GetPlayerVoiceVolume",                              // 修正名称
        //     (uint8_t*)(this->soundsystem.GetBaseAddress() + 0x062890),          // 修正地址
        //     [this](MulNX::Hook* hk, RegContext* ctx) {
        //         Steam64UID uid = ctx->rdx;
        //         // 若需修改返回值，可在回调后改变 ctx->xmm0
        //         return MulNX::Hook::Then::Continue;
        //     }).value();
        // this->hkSoundSystem001_GetPlayerVoiceVolume->Attach();
        });
    
    return true;
}