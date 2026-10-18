#pragma once
#include <Intro/CSModuleBase.hpp>

class SkinController final :public CSModuleBase {
    using CEconItemView_GetPaint_t = uint32_t(*)(CS2::C_EconItemView* pEconItemView);

    std::atomic<int> index{};

    std::unique_ptr<MulNX::Hook> hkGetPaint = nullptr;
    void Window();
    bool Init()override;

    std::optional<uint64_t> GetPaintIndex(CS2::C_EconItemView* pEconItemView);
};