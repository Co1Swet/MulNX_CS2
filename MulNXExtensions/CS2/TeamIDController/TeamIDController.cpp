#include "TeamIDController.hpp"
#include <MulNXExtensions/CS2/CSController/CSController.hpp>
#include <MulNXThirdParty/hlae/binutils.h>

bool TeamIDController::Init() {
    auto target = this->CS2()->panorama.GetTextRegion().FindRegion(MulNX::CS2::Signatures::CLayoutFile_LoadFromFile);
    this->hkCLayoutFile_LoadFromFile = MulNX::Hook::Create(target.Data(), 0, false,
        [this](RegContext* ctx, MulNX::Hook* hk) {
            return MulNX::Hook::Then::Continue;
        }).value();
    this->hkCLayoutFile_LoadFromFile->Attach();

    void** vtable = (void**)Afx::BinUtils::FindClassVtable(this->CS2()->panorama.hModule, ".?AVCStylePropertyWashColor@panorama@@", 0, 0);

    return true;
}