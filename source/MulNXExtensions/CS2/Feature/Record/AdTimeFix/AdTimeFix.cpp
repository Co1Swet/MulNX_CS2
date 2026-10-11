#include "AdTimeFix.hpp"

bool AdTimeFix::Init() {
    this->pThis = this;

    this->SubscribeSync("Hook/LoadLibraryExW/scenesystem.dll", [this](MulNX::Message& msg) {
        auto slot = this->CS2->scenesystem.FindIatSlot("tier0.dll", "Plat_FloatTime");
        this->hkScenesystem_Plat_FloatTime = MulNX::Memory::IatHook(slot, (void*)+[](void)->double {
            return pThis->OnGetTime();
            });
        this->pRawPlat_FloatTime = this->hkScenesystem_Plat_FloatTime.Original<Plat_FloatTime_t>();
        this->RegisterAttachIatHook(this->hkScenesystem_Plat_FloatTime, "Plat_FloatTime(Scenesystem)");
        });

    this->SubscribeSync("Hook/LoadLibraryExW/panorama.dll", [this](auto&&...) {
        auto slot = this->CS2->panorama.FindIatSlot("tier0.dll", "Plat_FloatTime");
        this->hkPanorama_Plat_FloatTime = MulNX::Memory::IatHook(slot, (void*)+[](void)->double {
            return pThis->OnGetTime();
            });
        this->RegisterAttachIatHook(this->hkPanorama_Plat_FloatTime, "Plat_FloatTime(Panorama)");
        });

    return true;
}

double AdTimeFix::OnGetTime(void) {
    auto time = this->pRawPlat_FloatTime();
    return time;
}