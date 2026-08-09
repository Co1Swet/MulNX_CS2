#pragma once
#include <cstdint>
#include <algorithm>

namespace MulNX {

    class ColorTran {
        // 内部统一存储为 0xRRGGBBAA (R 在高位，A 在低位)
        uint32_t rgba = 0;

    public:
        // ================= 输入（解析） =================
        inline void PraseF1(float r, float g, float b, float a) {
            auto toByte = [](float v) -> uint8_t {
                if (v < 0.0f) v = 0.0f;
                if (v > 1.0f) v = 1.0f;
                return static_cast<uint8_t>(v * 255.0f + 0.5f);
                };
            rgba = (uint32_t(toByte(r)) << 24)
                | (uint32_t(toByte(g)) << 16)
                | (uint32_t(toByte(b)) << 8)
                | (uint32_t(toByte(a)));
        }

        inline void PraseF255(float r, float g, float b, float a) {
            auto toByte = [](float v) -> uint8_t {
                if (v < 0.0f) v = 0.0f;
                if (v > 255.0f) v = 255.0f;
                return static_cast<uint8_t>(v + 0.5f);
                };
            rgba = (uint32_t(toByte(r)) << 24)
                | (uint32_t(toByte(g)) << 16)
                | (uint32_t(toByte(b)) << 8)
                | (uint32_t(toByte(a)));
        }

        inline void PraseU1(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
            rgba = (uint32_t(r) << 24)
                | (uint32_t(g) << 16)
                | (uint32_t(b) << 8)
                | (uint32_t(a));
        }

        inline void PraseU255RGBA(uint32_t rgbaVal) {
            rgba = rgbaVal;
        }

        inline void PraseU255ABGR(uint32_t abgr) {
            rgba = ((abgr & 0xFF000000) >> 24)   // A → 低位
                | ((abgr & 0x00FF0000) >> 8)    // B → B
                | ((abgr & 0x0000FF00) << 8)    // G → G
                | ((abgr & 0x000000FF) << 24);  // R → 高位
        }

        inline void PraseU255ARGB(uint32_t argb) {
            rgba = ((argb & 0x00FF0000) >> 8)    // R → 高位
                | ((argb & 0x0000FF00) << 8)    // G → G
                | ((argb & 0x000000FF) << 8)    // B → B
                | ((argb & 0xFF000000) >> 24);  // A → 低位
        }

        // ================= 输出 =================

        // 输出为 RGBA 格式 uint32_t (0xRRGGBBAA)
        inline uint32_t ToU255RGBA() const {
            return rgba;
        }

        // 输出 RGB 三个通道为 0～255 范围的浮点数
        inline void ToF255RGB(float& r, float& g, float& b) const {
            r = static_cast<float>(GetR());
            g = static_cast<float>(GetG());
            b = static_cast<float>(GetB());
        }

        // 输出 RGBA 四个通道为 0～255 范围的浮点数
        inline void ToF255RGBA(float& r, float& g, float& b, float& a) const {
            r = static_cast<float>(GetR());
            g = static_cast<float>(GetG());
            b = static_cast<float>(GetB());
            a = static_cast<float>(GetA());
        }

        // 输出为 0～1 范围的浮点数（四个参数）
        inline void ToF1(float& r, float& g, float& b, float& a) const {
            r = GetR() / 255.0f;
            g = GetG() / 255.0f;
            b = GetB() / 255.0f;
            a = GetA() / 255.0f;
        }

        // 输出为 0～255 的 uint8_t 整数（四个参数）
        inline void ToU1(uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) const {
            r = GetR();
            g = GetG();
            b = GetB();
            a = GetA();
        }

        // 单独获取各通道（方便直接使用）
        inline uint8_t GetR() const { return (rgba >> 24) & 0xFF; }
        inline uint8_t GetG() const { return (rgba >> 16) & 0xFF; }
        inline uint8_t GetB() const { return (rgba >> 8) & 0xFF; }
        inline uint8_t GetA() const { return rgba & 0xFF; }
    };

} // namespace MulNX