#include "CCStrike15ItemSystem.hpp"

bool CCStrike15ItemSystem::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](auto&&...) {
        this->pGetInner = (GetInner_t)this->CS2->client.GetTextRegion()
            .FindRegion(CS2::Signatures::Skin::CCStrike15ItemSystem::GetInner).Data();
        this->pFindPaintDefByIndex = (FindPaintDefByIndex_t)this->CS2->client.GetTextRegion()
            .FindRegion(CS2::Signatures::Skin::CCStrike15ItemSystem::FindPaintDefByIndex).Data();
        });

    return true;
}

CS2::CStrike15PaintDef* CCStrike15ItemSystem::FindPaintDefByIndex(int index) {
    auto pInner = this->pGetInner();
    return this->pFindPaintDefByIndex(pInner, index);
}