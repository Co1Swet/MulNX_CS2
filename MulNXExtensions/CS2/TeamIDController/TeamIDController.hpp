#pragma once
#include <MulNXExtensions/CS2/CSModuleBase.hpp>
#include <MulNXExtensions/WinExt/WinExt.hpp>
#include <set>
#include <cstdint>
#include <memory>

class TeamIDController final : public CSModuleBase {
public:
    bool Init() override;

    // 设置 T 方头顶颜色，a=0 即可隐藏
    void SetTColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
    // 设置 CT 方头顶颜色
    void SetCTColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
    // 恢复默认颜色
    void ResetToDefault();

private:
    // 向单个 WashColor 对象写入颜色 (偏移 +0x10)
    void WriteColor(uintptr_t obj, uint32_t rgba) {
        if (obj) *reinterpret_cast<uint32_t*>(obj + 0x10) = rgba;
    }

    // Hook 句柄
    std::unique_ptr<MulNX::Hook> hkLoadFromFile_;
    std::unique_ptr<MulNX::Hook> hkWashColorParse_;
    std::unique_ptr<MulNX::Hook> hkWashColorClone_;

    // 是否正在加载 hudreticle.xml
    bool inHudReticle_ = false;

    // 收集到的 WashColor 对象地址（this 指针）
    std::set<uintptr_t> tWashColors_;
    std::set<uintptr_t> ctWashColors_;
};