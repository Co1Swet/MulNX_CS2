// ImGuiStyleYamlSerialization.h
// Dear ImGui v1.92.7 -> YAML serialization (function-based, no template specialization)
#pragma once

#include <yaml-cpp/yaml.h>
#include <MulNXThirdParty/imgui_d11/imgui.h>
#include <unordered_map>
#include <string>

namespace ImGuiYaml {

    // ---------- ImVec2 ----------
    inline YAML::Node EncodeImVec2(const ImVec2& v) {
        YAML::Node node;
        node.push_back(v.x);
        node.push_back(v.y);
        return node;
    }

    inline bool DecodeImVec2(const YAML::Node& node, ImVec2& v) {
        if (!node.IsSequence() || node.size() != 2) return false;
        v.x = node[0].as<float>();
        v.y = node[1].as<float>();
        return true;
    }

    // ---------- ImVec4 ----------
    inline YAML::Node EncodeImVec4(const ImVec4& v) {
        YAML::Node node;
        node.push_back(v.x);
        node.push_back(v.y);
        node.push_back(v.z);
        node.push_back(v.w);
        return node;
    }

    inline bool DecodeImVec4(const YAML::Node& node, ImVec4& v) {
        if (!node.IsSequence() || node.size() != 4) return false;
        v.x = node[0].as<float>();
        v.y = node[1].as<float>();
        v.z = node[2].as<float>();
        v.w = node[3].as<float>();
        return true;
    }

    // ---------- ImGuiDir ----------
    inline YAML::Node EncodeImGuiDir(ImGuiDir dir) {
        static const std::unordered_map<ImGuiDir, std::string> map = {
            {ImGuiDir_None, "None"},
            {ImGuiDir_Left, "Left"},
            {ImGuiDir_Right, "Right"},
            {ImGuiDir_Up, "Up"},
            {ImGuiDir_Down, "Down"}
        };
        auto it = map.find(dir);
        return YAML::Node(it != map.end() ? it->second : "Unknown");
    }

    inline bool DecodeImGuiDir(const YAML::Node& node, ImGuiDir& dir) {
        if (!node.IsScalar()) return false;
        std::string str = node.Scalar();
        static const std::unordered_map<std::string, ImGuiDir> map = {
            {"None", ImGuiDir_None},
            {"Left", ImGuiDir_Left},
            {"Right", ImGuiDir_Right},
            {"Up", ImGuiDir_Up},
            {"Down", ImGuiDir_Down}
        };
        auto it = map.find(str);
        if (it != map.end()) {
            dir = it->second;
            return true;
        }
        return false;
    }

    // ---------- ImGuiTreeNodeFlags (bitmask) ----------
    inline YAML::Node EncodeImGuiTreeNodeFlags(ImGuiTreeNodeFlags flags) {
        return YAML::Node(static_cast<int>(flags));
    }
    inline bool DecodeImGuiTreeNodeFlags(const YAML::Node& node, ImGuiTreeNodeFlags& flags) {
        if (!node.IsScalar()) return false;
        flags = static_cast<ImGuiTreeNodeFlags>(node.as<int>());
        return true;
    }

    // ---------- ImGuiHoveredFlags (bitmask) ----------
    inline YAML::Node EncodeImGuiHoveredFlags(ImGuiHoveredFlags flags) {
        return YAML::Node(static_cast<int>(flags));
    }
    inline bool DecodeImGuiHoveredFlags(const YAML::Node& node, ImGuiHoveredFlags& flags) {
        if (!node.IsScalar()) return false;
        flags = static_cast<ImGuiHoveredFlags>(node.as<int>());
        return true;
    }

    // ---------- Main: Style <-> YAML Node ----------
    inline void StyleToYaml(const ImGuiStyle& style, YAML::Node& out) {
        // Font scaling
        out["FontSizeBase"] = style.FontSizeBase;
        out["FontScaleMain"] = style.FontScaleMain;
        out["FontScaleDpi"] = style.FontScaleDpi;

        // General
        out["Alpha"] = style.Alpha;
        out["DisabledAlpha"] = style.DisabledAlpha;

        // Window
        out["WindowPadding"] = EncodeImVec2(style.WindowPadding);
        out["WindowRounding"] = style.WindowRounding;
        out["WindowBorderSize"] = style.WindowBorderSize;
        out["WindowBorderHoverPadding"] = style.WindowBorderHoverPadding;
        out["WindowMinSize"] = EncodeImVec2(style.WindowMinSize);
        out["WindowTitleAlign"] = EncodeImVec2(style.WindowTitleAlign);
        out["WindowMenuButtonPosition"] = EncodeImGuiDir(style.WindowMenuButtonPosition);

        // Child
        out["ChildRounding"] = style.ChildRounding;
        out["ChildBorderSize"] = style.ChildBorderSize;

        // Popup
        out["PopupRounding"] = style.PopupRounding;
        out["PopupBorderSize"] = style.PopupBorderSize;

        // Frame
        out["FramePadding"] = EncodeImVec2(style.FramePadding);
        out["FrameRounding"] = style.FrameRounding;
        out["FrameBorderSize"] = style.FrameBorderSize;

        // Spacing / Layout
        out["ItemSpacing"] = EncodeImVec2(style.ItemSpacing);
        out["ItemInnerSpacing"] = EncodeImVec2(style.ItemInnerSpacing);
        out["CellPadding"] = EncodeImVec2(style.CellPadding);
        out["TouchExtraPadding"] = EncodeImVec2(style.TouchExtraPadding);
        out["IndentSpacing"] = style.IndentSpacing;
        out["ColumnsMinSpacing"] = style.ColumnsMinSpacing;
        out["ScrollbarSize"] = style.ScrollbarSize;
        out["ScrollbarRounding"] = style.ScrollbarRounding;
        out["ScrollbarPadding"] = style.ScrollbarPadding;
        out["GrabMinSize"] = style.GrabMinSize;
        out["GrabRounding"] = style.GrabRounding;
        out["LogSliderDeadzone"] = style.LogSliderDeadzone;

        // Images
        out["ImageRounding"] = style.ImageRounding;
        out["ImageBorderSize"] = style.ImageBorderSize;

        // Tabs
        out["TabRounding"] = style.TabRounding;
        out["TabBorderSize"] = style.TabBorderSize;
        out["TabMinWidthBase"] = style.TabMinWidthBase;
        out["TabMinWidthShrink"] = style.TabMinWidthShrink;
        out["TabCloseButtonMinWidthSelected"] = style.TabCloseButtonMinWidthSelected;
        out["TabCloseButtonMinWidthUnselected"] = style.TabCloseButtonMinWidthUnselected;
        out["TabBarBorderSize"] = style.TabBarBorderSize;
        out["TabBarOverlineSize"] = style.TabBarOverlineSize;

        // Tables
        out["TableAngledHeadersAngle"] = style.TableAngledHeadersAngle;
        out["TableAngledHeadersTextAlign"] = EncodeImVec2(style.TableAngledHeadersTextAlign);

        // Tree lines
        out["TreeLinesFlags"] = EncodeImGuiTreeNodeFlags(style.TreeLinesFlags);
        out["TreeLinesSize"] = style.TreeLinesSize;
        out["TreeLinesRounding"] = style.TreeLinesRounding;

        // Drag & Drop
        out["DragDropTargetRounding"] = style.DragDropTargetRounding;
        out["DragDropTargetBorderSize"] = style.DragDropTargetBorderSize;
        out["DragDropTargetPadding"] = style.DragDropTargetPadding;

        // Color marker
        out["ColorMarkerSize"] = style.ColorMarkerSize;
        out["ColorButtonPosition"] = EncodeImGuiDir(style.ColorButtonPosition);

        // Text alignment
        out["ButtonTextAlign"] = EncodeImVec2(style.ButtonTextAlign);
        out["SelectableTextAlign"] = EncodeImVec2(style.SelectableTextAlign);

        // Separator
        out["SeparatorSize"] = style.SeparatorSize;
        out["SeparatorTextBorderSize"] = style.SeparatorTextBorderSize;
        out["SeparatorTextAlign"] = EncodeImVec2(style.SeparatorTextAlign);
        out["SeparatorTextPadding"] = EncodeImVec2(style.SeparatorTextPadding);

        // Display / safe area
        out["DisplayWindowPadding"] = EncodeImVec2(style.DisplayWindowPadding);
        out["DisplaySafeAreaPadding"] = EncodeImVec2(style.DisplaySafeAreaPadding);

        // Mouse cursor scale
        out["MouseCursorScale"] = style.MouseCursorScale;

        // Antialiasing
        out["AntiAliasedLines"] = style.AntiAliasedLines;
        out["AntiAliasedLinesUseTex"] = style.AntiAliasedLinesUseTex;
        out["AntiAliasedFill"] = style.AntiAliasedFill;

        // Tessellation
        out["CurveTessellationTol"] = style.CurveTessellationTol;
        out["CircleTessellationMaxError"] = style.CircleTessellationMaxError;

        // Behaviors
        out["HoverStationaryDelay"] = style.HoverStationaryDelay;
        out["HoverDelayShort"] = style.HoverDelayShort;
        out["HoverDelayNormal"] = style.HoverDelayNormal;
        out["HoverFlagsForTooltipMouse"] = EncodeImGuiHoveredFlags(style.HoverFlagsForTooltipMouse);
        out["HoverFlagsForTooltipNav"] = EncodeImGuiHoveredFlags(style.HoverFlagsForTooltipNav);

        // Internal (optional but keep)
        out["_MainScale"] = style._MainScale;
        out["_NextFrameFontSizeBase"] = style._NextFrameFontSizeBase;

        // Colors
        YAML::Node colorsNode;
        for (int i = 0; i < ImGuiCol_COUNT; ++i) {
            ImGuiCol colEnum = static_cast<ImGuiCol>(i);
            const char* name = ImGui::GetStyleColorName(colEnum);
            colorsNode[name] = EncodeImVec4(style.Colors[i]);
        }
        out["Colors"] = colorsNode;
    }

    // 将字符串颜色名或标量 YAML 节点转为 ImGuiCol 枚举
    inline bool DecodeImGuiCol(const YAML::Node& node, ImGuiCol& col) {
        if (!node.IsScalar()) return false;
        std::string name = node.Scalar();

        // 一次性构建字符串 -> 枚举的静态映射（利用 ImGui 的 API 确保版本兼容）
        static std::unordered_map<std::string, ImGuiCol> map;
        if (map.empty()) {
            for (int i = 0; i < ImGuiCol_COUNT; ++i) {
                ImGuiCol c = static_cast<ImGuiCol>(i);
                map[ImGui::GetStyleColorName(c)] = c;
            }
        }

        auto it = map.find(name);
        if (it != map.end()) {
            col = it->second;
            return true;
        }
        return false;
    }

    inline bool YamlToStyle(const YAML::Node& node, ImGuiStyle& style) {
        if (!node.IsMap()) return false;

        // Helper to read scalar
        auto readFloat = [&](const char* key, float& out) {
            if (node[key]) out = node[key].as<float>();
            };
        auto readBool = [&](const char* key, bool& out) {
            if (node[key]) out = node[key].as<bool>();
            };
        auto readVec2 = [&](const char* key, ImVec2& out) {
            if (node[key]) DecodeImVec2(node[key], out);
            };
        auto readDir = [&](const char* key, ImGuiDir& out) {
            if (node[key]) DecodeImGuiDir(node[key], out);
            };
        auto readFlagsTree = [&](const char* key, ImGuiTreeNodeFlags& out) {
            if (node[key]) DecodeImGuiTreeNodeFlags(node[key], out);
            };
        auto readFlagsHover = [&](const char* key, ImGuiHoveredFlags& out) {
            if (node[key]) DecodeImGuiHoveredFlags(node[key], out);
            };

        // Font
        readFloat("FontSizeBase", style.FontSizeBase);
        readFloat("FontScaleMain", style.FontScaleMain);
        readFloat("FontScaleDpi", style.FontScaleDpi);

        readFloat("Alpha", style.Alpha);
        readFloat("DisabledAlpha", style.DisabledAlpha);

        readVec2("WindowPadding", style.WindowPadding);
        readFloat("WindowRounding", style.WindowRounding);
        readFloat("WindowBorderSize", style.WindowBorderSize);
        readFloat("WindowBorderHoverPadding", style.WindowBorderHoverPadding);
        readVec2("WindowMinSize", style.WindowMinSize);
        readVec2("WindowTitleAlign", style.WindowTitleAlign);
        readDir("WindowMenuButtonPosition", style.WindowMenuButtonPosition);

        readFloat("ChildRounding", style.ChildRounding);
        readFloat("ChildBorderSize", style.ChildBorderSize);

        readFloat("PopupRounding", style.PopupRounding);
        readFloat("PopupBorderSize", style.PopupBorderSize);

        readVec2("FramePadding", style.FramePadding);
        readFloat("FrameRounding", style.FrameRounding);
        readFloat("FrameBorderSize", style.FrameBorderSize);

        readVec2("ItemSpacing", style.ItemSpacing);
        readVec2("ItemInnerSpacing", style.ItemInnerSpacing);
        readVec2("CellPadding", style.CellPadding);
        readVec2("TouchExtraPadding", style.TouchExtraPadding);
        readFloat("IndentSpacing", style.IndentSpacing);
        readFloat("ColumnsMinSpacing", style.ColumnsMinSpacing);
        readFloat("ScrollbarSize", style.ScrollbarSize);
        readFloat("ScrollbarRounding", style.ScrollbarRounding);
        readFloat("ScrollbarPadding", style.ScrollbarPadding);
        readFloat("GrabMinSize", style.GrabMinSize);
        readFloat("GrabRounding", style.GrabRounding);
        readFloat("LogSliderDeadzone", style.LogSliderDeadzone);

        readFloat("ImageRounding", style.ImageRounding);
        readFloat("ImageBorderSize", style.ImageBorderSize);

        readFloat("TabRounding", style.TabRounding);
        readFloat("TabBorderSize", style.TabBorderSize);
        readFloat("TabMinWidthBase", style.TabMinWidthBase);
        readFloat("TabMinWidthShrink", style.TabMinWidthShrink);
        readFloat("TabCloseButtonMinWidthSelected", style.TabCloseButtonMinWidthSelected);
        readFloat("TabCloseButtonMinWidthUnselected", style.TabCloseButtonMinWidthUnselected);
        readFloat("TabBarBorderSize", style.TabBarBorderSize);
        readFloat("TabBarOverlineSize", style.TabBarOverlineSize);

        readFloat("TableAngledHeadersAngle", style.TableAngledHeadersAngle);
        readVec2("TableAngledHeadersTextAlign", style.TableAngledHeadersTextAlign);

        readFlagsTree("TreeLinesFlags", style.TreeLinesFlags);
        readFloat("TreeLinesSize", style.TreeLinesSize);
        readFloat("TreeLinesRounding", style.TreeLinesRounding);

        readFloat("DragDropTargetRounding", style.DragDropTargetRounding);
        readFloat("DragDropTargetBorderSize", style.DragDropTargetBorderSize);
        readFloat("DragDropTargetPadding", style.DragDropTargetPadding);

        readFloat("ColorMarkerSize", style.ColorMarkerSize);
        readDir("ColorButtonPosition", style.ColorButtonPosition);

        readVec2("ButtonTextAlign", style.ButtonTextAlign);
        readVec2("SelectableTextAlign", style.SelectableTextAlign);

        readFloat("SeparatorSize", style.SeparatorSize);
        readFloat("SeparatorTextBorderSize", style.SeparatorTextBorderSize);
        readVec2("SeparatorTextAlign", style.SeparatorTextAlign);
        readVec2("SeparatorTextPadding", style.SeparatorTextPadding);

        readVec2("DisplayWindowPadding", style.DisplayWindowPadding);
        readVec2("DisplaySafeAreaPadding", style.DisplaySafeAreaPadding);

        readFloat("MouseCursorScale", style.MouseCursorScale);

        readBool("AntiAliasedLines", style.AntiAliasedLines);
        readBool("AntiAliasedLinesUseTex", style.AntiAliasedLinesUseTex);
        readBool("AntiAliasedFill", style.AntiAliasedFill);

        readFloat("CurveTessellationTol", style.CurveTessellationTol);
        readFloat("CircleTessellationMaxError", style.CircleTessellationMaxError);

        readFloat("HoverStationaryDelay", style.HoverStationaryDelay);
        readFloat("HoverDelayShort", style.HoverDelayShort);
        readFloat("HoverDelayNormal", style.HoverDelayNormal);
        readFlagsHover("HoverFlagsForTooltipMouse", style.HoverFlagsForTooltipMouse);
        readFlagsHover("HoverFlagsForTooltipNav", style.HoverFlagsForTooltipNav);

        readFloat("_MainScale", style._MainScale);
        readFloat("_NextFrameFontSizeBase", style._NextFrameFontSizeBase);

        // Colors
        if (node["Colors"] && node["Colors"].IsMap()) {
            YAML::Node colorsNode = node["Colors"];
            for (auto it = colorsNode.begin(); it != colorsNode.end(); ++it) {
                std::string key = it->first.as<std::string>();
                ImGuiCol colEnum;
                if (DecodeImGuiCol(YAML::Node(key), colEnum)) {
                    DecodeImVec4(it->second, style.Colors[colEnum]);
                }
            }
        }
        return true;
    }

} // namespace ImGuiYaml