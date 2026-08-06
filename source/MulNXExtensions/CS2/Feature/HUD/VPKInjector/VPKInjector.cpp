#include "VPKInjector.hpp"

bool VPKInjector::Init() {

    return true;
}

std::optional<MulNX::Hook::Then> VPKInjector::OnCreateFileW(MulNX::Hook* hk, RegContext* ctx) {

    return std::nullopt;
}