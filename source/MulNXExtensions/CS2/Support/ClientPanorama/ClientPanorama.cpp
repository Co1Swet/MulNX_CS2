#include "ClientPanorama.hpp"

bool ClientPanorama::Init() {
    this->SubscribeSync("Hook/LoadLibraryExW/client.dll", [this](auto&&...) {
        this->pFindHudElement = (FindHudElement_t)this->CS2->client.GetTextRegion()
            .FindRegion(CS2::Signatures::ClientPanorama::FindHudElement).Data();
        });

    return true;
}