#include <MulNXThirdParty/imgui_d11/imgui.h>
#include <stdio.h>
#include <cstdint>
#include <atomic>

class BitMaskEditor {
public:
    BitMaskEditor() : mask(0) {}

    // 渲染 64 个复选框，按 8 位一组分行
    void Render(const char* label = "Bit Mask") {
        ImGui::Text("%s", label);
        for (int byte = 0; byte < 8; ++byte) {
            ImGui::PushID(byte);
            for (int bit = 0; bit < 8; ++bit) {
                int bitIndex = byte * 8 + bit;

                // 1. 以 relaxed 顺序读取当前位状态（仅用于显示）
                bool isSet = (mask.load(std::memory_order_relaxed) >> bitIndex) & 1ULL;

                char checkboxLabel[16];
                snprintf(checkboxLabel, sizeof(checkboxLabel), "%d", bitIndex);

                // 2. 复选框交互
                if (ImGui::Checkbox(checkboxLabel, &isSet)) {
                    // 用户更改了该位，直接根据新值进行原子置位/清零
                    if (isSet) {
                        mask.fetch_or(1ULL << bitIndex, std::memory_order_relaxed);
                    }
                    else {
                        mask.fetch_and(~(1ULL << bitIndex), std::memory_order_relaxed);
                    }
                }

                if (bit < 7) ImGui::SameLine();
            }
            ImGui::PopID();
        }
        // 显示当前掩码（快照）
        ImGui::Text("Mask: 0x%016llX", mask.load(std::memory_order_relaxed));
    }

    // 将当前掩码写入目标 uint64_t（原子地读取）
    void Apply(uint64_t& target) const {
        target = mask.load(std::memory_order_relaxed);
    }

    // 获取当前掩码
    uint64_t GetMask() const {
        return mask.load(std::memory_order_relaxed);
    }

    // 外部设置掩码（例如初始化）
    void SetMask(uint64_t newMask) {
        mask.store(newMask, std::memory_order_relaxed);
    }

private:
    std::atomic<uint64_t> mask;
};