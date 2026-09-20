#pragma once
#include <MulNX/Common/Message.hpp>
#include <yaml-cpp/yaml.h>
#include <atomic>

namespace MulNX {
    // 按键绑定数据包：包含修饰键、主键码、连击数及有效性标志
    class alignas(8) KeyCheckPack {
    public:
        bool Usable = false;
        bool Ctrl = false;
        bool Shift = false;
        bool Alt = false;

        uint8_t vkCode = 0;   // Windows 虚拟键码
        uint8_t ComboClick = 0;     // 连击次数（1~255）

        std::string GetMsg() const;
        void Refresh();
        // 第一个无代表无修改，第一个有代表有变化，第二个代表是否按下确认修改按钮
        std::pair<std::optional<KeyCheckPack>, bool> DebugWindow(const char* windowName, std::atomic<bool>& openWindow)const;

        inline static KeyCheckPack ParseFromMessage(MulNX::Message& msg) {
            auto&& [u, c, s, a, vk, cc] = msg.Access<bool, bool, bool, bool, uint8_t, uint8_t>();
            KeyCheckPack KCP{};
            KCP.Usable = u;
            KCP.Ctrl = c;
            KCP.Shift = s;
            KCP.Alt = a;
            KCP.vkCode = vk;
            KCP.ComboClick = cc;
            return KCP;
        }
        inline void WriteToMessage(MulNX::Message& msg) {
            auto&& [u, c, s, a, vk, cc] = msg.Access<bool, bool, bool, bool, uint8_t, uint8_t>();
            u = this->Usable;
            c = this->Ctrl;
            s = this->Shift;
            a = this->Alt;
            vk = this->vkCode;
            cc = this->ComboClick;
        }
    };
    static_assert(std::atomic<KeyCheckPack>::is_always_lock_free);
}

namespace YAML {
    template<>
    struct convert<MulNX::KeyCheckPack> {
        static Node encode(const MulNX::KeyCheckPack& KCP) {
            Node node;
            node["usable"] = KCP.Usable;
            node["ctrl"] = KCP.Ctrl;
            node["shift"] = KCP.Shift;
            node["alt"] = KCP.Alt;
            node["vkCode"] = static_cast<int>(KCP.vkCode);
            node["comboClick"] = static_cast<int>(KCP.ComboClick);
            return node;
        }
        static bool decode(const Node& node, MulNX::KeyCheckPack& KCP) {
            if (!node.IsMap()) return false;
            try {
                MulNX::KeyCheckPack temp;
                temp.Usable = node["usable"].as<bool>();
                temp.Ctrl = node["ctrl"].as<bool>();
                temp.Shift = node["shift"].as<bool>();
                temp.Alt = node["alt"].as<bool>();
                temp.vkCode = static_cast<uint8_t>(node["vkCode"].as<int>());
                temp.ComboClick = static_cast<uint8_t>(node["comboClick"].as<int>());
                KCP = std::move(temp);
                return true;
            }
            catch (const YAML::Exception&) {
                return false;
            }
        }
    };
}