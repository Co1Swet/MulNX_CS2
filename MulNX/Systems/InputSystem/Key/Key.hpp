#pragma once

#include <yaml-cpp/yaml.h>
#include <atomic>

namespace MulNX {
    // 按键绑定数据包：包含修饰键、主键码、连击数及有效性标志
    class KeyCheckPack {
    public:
        bool Usable = false;
        bool Ctrl = false;
        bool Shift = false;
        bool Alt = false;

        unsigned char vkCode = 0;   // Windows 虚拟键码
        uint8_t ComboClick = 0;     // 连击次数（1~255）

        std::string GetMsg() const;
        void Refresh();
        bool DebugWindow(std::atomic<bool>& openWindow);
    };
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
                temp.vkCode = static_cast<unsigned char>(node["vkCode"].as<unsigned int>());
                temp.ComboClick = static_cast<uint8_t>(node["comboClick"].as<unsigned int>());
                KCP = std::move(temp);
                return true;
            }
            catch (const YAML::Exception&) {
                return false;
            }
        }
    };
}