#include "Key.hpp"
#include <MulNX/Base/UI/UI.hpp>
#include <iomanip>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <sstream>
#include <atomic>

namespace {
    // 虚拟键码 -> 显示名称映射表（Windows 虚拟键码，参考 WinUser.h）
    const std::unordered_map<unsigned char, std::string> kKeyMap = {
        // 字母键 A-Z
        {0x41, "A"}, {0x42, "B"}, {0x43, "C"}, {0x44, "D"}, {0x45, "E"},
        {0x46, "F"}, {0x47, "G"}, {0x48, "H"}, {0x49, "I"}, {0x4A, "J"},
        {0x4B, "K"}, {0x4C, "L"}, {0x4D, "M"}, {0x4E, "N"}, {0x4F, "O"},
        {0x50, "P"}, {0x51, "Q"}, {0x52, "R"}, {0x53, "S"}, {0x54, "T"},
        {0x55, "U"}, {0x56, "V"}, {0x57, "W"}, {0x58, "X"}, {0x59, "Y"},
        {0x5A, "Z"},

        // 数字键 0-9
        {0x30, "0"}, {0x31, "1"}, {0x32, "2"}, {0x33, "3"}, {0x34, "4"},
        {0x35, "5"}, {0x36, "6"}, {0x37, "7"}, {0x38, "8"}, {0x39, "9"},

        // 功能键 F1~F24
        {0x70, "F1"},  {0x71, "F2"},  {0x72, "F3"},  {0x73, "F4"},
        {0x74, "F5"},  {0x75, "F6"},  {0x76, "F7"},  {0x77, "F8"},
        {0x78, "F9"},  {0x79, "F10"}, {0x7A, "F11"}, {0x7B, "F12"},
        {0x7C, "F13"}, {0x7D, "F14"}, {0x7E, "F15"}, {0x7F, "F16"},
        {0x80, "F17"}, {0x81, "F18"}, {0x82, "F19"}, {0x83, "F20"},
        {0x84, "F21"}, {0x85, "F22"}, {0x86, "F23"}, {0x87, "F24"},

        // 方向键
        {0x25, "←"}, {0x26, "↑"}, {0x27, "→"}, {0x28, "↓"},

        // 编辑键
        {0x2D, "Insert"}, {0x2E, "Delete"}, {0x24, "Home"}, {0x23, "End"},
        {0x21, "PageUp"}, {0x22, "PageDown"},

        // 控制键
        {0x20, "空格"},   {0x0D, "回车"},   {0x1B, "ESC"},    {0x09, "Tab"},
        {0x2C, "PrintScreen"}, {0x90, "NumLock"}, {0x91, "ScrollLock"}, {0x14, "CapsLock"},

        // 小键盘数字键（NumLock 开启时）
        {0x60, "Num0"}, {0x61, "Num1"}, {0x62, "Num2"}, {0x63, "Num3"},
        {0x64, "Num4"}, {0x65, "Num5"}, {0x66, "Num6"}, {0x67, "Num7"},
        {0x68, "Num8"}, {0x69, "Num9"},

        // 小键盘运算符
        {0x6A, "Num*"}, {0x6B, "Num+"}, {0x6D, "Num-"}, {0x6F, "Num/"},
        {0x6E, "Num."},
    };

    // 将映射表转换为排序后的 vector（按显示名称排序，便于 UI 查找）
    std::vector<std::pair<unsigned char, std::string>> BuildSortedKeyList() {
        std::vector<std::pair<unsigned char, std::string>> vec;
        vec.reserve(kKeyMap.size());
        for (const auto& [code, name] : kKeyMap) {
            vec.emplace_back(code, name);
        }
        std::sort(vec.begin(), vec.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; });
        return vec;
    }

    const std::vector<std::pair<unsigned char, std::string>>& GetSortedKeyList() {
        static const auto list = BuildSortedKeyList();
        return list;
    }
} // anonymous namespace

std::string MulNX::KeyCheckPack::GetMsg() const {
    std::ostringstream oss;

    if (Ctrl)  oss << "Ctrl + ";
    if (Shift) oss << "Shift + ";
    if (Alt)   oss << "Alt + ";

    auto it = kKeyMap.find(vkCode);
    if (it != kKeyMap.end()) {
        oss << it->second;
    }
    else {
        // 未在映射表中的键（罕见）显示十六进制码
        oss << "键码[0x" << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(vkCode) << "]";
    }

    oss << "x" << static_cast<int>(ComboClick);
    return oss.str();
}

void MulNX::KeyCheckPack::Refresh() {
    Usable = (vkCode != 0) && (ComboClick != 0);
}

bool MulNX::KeyCheckPack::DebugWindow(std::atomic<bool>& openWindow) {
    auto w = MulNX::UI::RAIIWindow("按键绑定", openWindow);
    if (!w) return false;

    ImGui::Text(("当前绑键：" + GetMsg()).c_str());
    ImGui::Separator();

    // 修饰键复选框
    ImGui::Checkbox("Ctrl", &Ctrl);
    ImGui::SameLine();
    ImGui::Checkbox("Shift", &Shift);
    ImGui::SameLine();
    ImGui::Checkbox("Alt", &Alt);

    // ----- 动态生成主按键下拉菜单 -----
    const auto& keyList = GetSortedKeyList();
    int currentIndex = 0;
    for (size_t i = 0; i < keyList.size(); ++i) {
        if (keyList[i].first == vkCode) {
            currentIndex = static_cast<int>(i);
            break;
        }
    }

    ImGui::Text("按键:");
    ImGui::SameLine();
    // 构建 const char* 数组以适配 ImGui::Combo
    std::vector<const char*> keyNames;
    keyNames.reserve(keyList.size());
    for (const auto& pair : keyList) {
        keyNames.push_back(pair.second.c_str());
    }

    if (ImGui::Combo("##KeyCombo", &currentIndex, keyNames.data(), static_cast<int>(keyNames.size()))) {
        vkCode = keyList[currentIndex].first;
    }

    // 连击数输入（1~255）
    int combo = static_cast<int>(ComboClick);
    ImGui::Text("连击数:");
    ImGui::SameLine();
    if (ImGui::InputInt("##ComboClick", &combo, 1, 5)) {
        combo = std::clamp(combo, 1, 255);
        ComboClick = static_cast<uint8_t>(combo);
    }

    if (ImGui::Button("确认修改")) {
        Refresh();
        return true;
    }

    return false;
}