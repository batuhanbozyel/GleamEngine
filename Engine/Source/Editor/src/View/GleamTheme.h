#pragma once
#include <imgui.h>

// GleamEngine editor theme — "Graphite & Gleam"
// Values are sRGB. If ImGui renders into an sRGB swapchain/RTV (D3D12 *_SRGB format,
// MTLPixelFormatBGRA8Unorm_sRGB), convert to linear before use or the UI will look washed out.

namespace Gleam::Theme
{
    constexpr ImVec4 Hex(unsigned rgb, float a = 1.0f)
    {
        return ImVec4(((rgb >> 16) & 0xFF) / 255.0f, ((rgb >> 8) & 0xFF) / 255.0f, (rgb & 0xFF) / 255.0f, a);
    }

    // Surfaces (darkest -> lightest)
    constexpr ImVec4 TitleBar     = Hex(0x0A0B0C);
    constexpr ImVec4 Background   = Hex(0x0E0F11); // window / dockspace / viewport letterbox
    constexpr ImVec4 Panel        = Hex(0x131417); // side panels, inspector, hierarchy
    constexpr ImVec4 Selected     = Hex(0x1D1F24); // selected row background
    constexpr ImVec4 Control      = Hex(0x1F2227); // buttons, frames, badges
    constexpr ImVec4 ControlHover = Hex(0x262A30);
    constexpr ImVec4 ControlActive= Hex(0x2E3238);
    constexpr ImVec4 SectionHeader= Hex(0x1A1C20); // inspector component headers
    constexpr ImVec4 ViewportBg   = Hex(0x16181C);

    // Lines
    constexpr ImVec4 RowDivider   = Hex(0x1C1E22);
    constexpr ImVec4 Divider      = Hex(0x22252A); // panel separators
    constexpr ImVec4 Border       = Hex(0x2A2D33); // control borders
    constexpr ImVec4 BorderStrong = Hex(0x2E3137);

    // Text
    constexpr ImVec4 Text         = Hex(0xE9E7E2);
    constexpr ImVec4 TextBadge    = Hex(0xC9CBCF);
    constexpr ImVec4 TextSecondary= Hex(0xB9BBC0);
    constexpr ImVec4 TextMuted    = Hex(0x9A9DA4);
    constexpr ImVec4 TextDim      = Hex(0x8A8E96); // captions, section headers, disabled

    // Accent
    constexpr ImVec4 Accent       = Hex(0xF2B544);
    constexpr ImVec4 AccentHover  = Hex(0xF5C266);
    constexpr ImVec4 AccentActive = Hex(0xD99E32);
    constexpr ImVec4 OnAccent     = Hex(0x1A1405); // text on accent fills

    // Status
    constexpr ImVec4 Info         = Hex(0x6FB7F2);
    constexpr ImVec4 Success      = Hex(0x8BD17C);
    constexpr ImVec4 Warning      = Hex(0xE58A6A);
    constexpr ImVec4 Error        = Hex(0xE5605A);

    // Asset / entity kinds
    constexpr ImVec4 Purple       = Hex(0xA08BE5);
    constexpr ImVec4 Pink         = Hex(0xD98AC4);
    constexpr ImVec4 Teal         = Hex(0x5CC8B8);

    // Viewport / gizmos
    constexpr ImVec4 SelectionOutline = Accent;
    constexpr ImVec4 HoverOutline     = Hex(0xF2B544, 0.45f);
    constexpr ImVec4 AxisX            = Hex(0xE5605A);
    constexpr ImVec4 AxisY            = Hex(0x8BD17C);
    constexpr ImVec4 AxisZ            = Hex(0x6FB7F2);
    constexpr ImVec4 GridMinor        = Hex(0x2A2D33, 0.5f);
    constexpr ImVec4 GridMajor        = Hex(0x3A3E46, 0.8f);

    inline void Apply()
    {
        ImGuiStyle& s = ImGui::GetStyle();
        s.WindowRounding    = 0.0f;
        s.ChildRounding     = 6.0f;
        s.FrameRounding     = 6.0f;
        s.PopupRounding     = 6.0f;
        s.GrabRounding      = 4.0f;
        s.TabRounding       = 4.0f;
        s.ScrollbarRounding = 6.0f;
        s.WindowBorderSize  = 1.0f;
        s.FrameBorderSize   = 1.0f;
        s.WindowPadding     = ImVec2(12, 12);
        s.FramePadding      = ImVec2(10, 6);
        s.ItemSpacing       = ImVec2(8, 8);
        s.ScrollbarSize     = 12.0f;
        s.TabBarBorderSize  = 1.0f;
        s.TabBarOverlineSize= 2.0f;
        s.DockingSeparatorSize = 1.0f;

        ImVec4* c = s.Colors;
        c[ImGuiCol_Text]                 = Text;
        c[ImGuiCol_TextDisabled]         = TextDim;
        c[ImGuiCol_WindowBg]             = Background;
        c[ImGuiCol_ChildBg]              = Panel;
        c[ImGuiCol_PopupBg]              = Panel;
        c[ImGuiCol_Border]               = Divider;
        c[ImGuiCol_BorderShadow]         = Hex(0x000000, 0.0f);

        c[ImGuiCol_FrameBg]              = Background;
        c[ImGuiCol_FrameBgHovered]       = Control;
        c[ImGuiCol_FrameBgActive]        = ControlHover;

        c[ImGuiCol_TitleBg]              = TitleBar;
        c[ImGuiCol_TitleBgActive]        = TitleBar;
        c[ImGuiCol_TitleBgCollapsed]     = TitleBar;
        c[ImGuiCol_MenuBarBg]            = TitleBar;

        c[ImGuiCol_ScrollbarBg]          = Hex(0x000000, 0.0f);
        c[ImGuiCol_ScrollbarGrab]        = Control;
        c[ImGuiCol_ScrollbarGrabHovered] = ControlHover;
        c[ImGuiCol_ScrollbarGrabActive]  = ControlActive;

        c[ImGuiCol_CheckMark]            = Accent;
        c[ImGuiCol_SliderGrab]           = Accent;
        c[ImGuiCol_SliderGrabActive]     = AccentActive;

        c[ImGuiCol_Button]               = Control;
        c[ImGuiCol_ButtonHovered]        = ControlHover;
        c[ImGuiCol_ButtonActive]         = ControlActive;

        c[ImGuiCol_Header]               = Selected;              // selected tree/list item
        c[ImGuiCol_HeaderHovered]        = Control;
        c[ImGuiCol_HeaderActive]         = ControlHover;

        c[ImGuiCol_Separator]            = Divider;
        c[ImGuiCol_SeparatorHovered]     = AccentHover;
        c[ImGuiCol_SeparatorActive]      = Accent;

        c[ImGuiCol_ResizeGrip]           = Hex(0x000000, 0.0f);
        c[ImGuiCol_ResizeGripHovered]    = AccentHover;
        c[ImGuiCol_ResizeGripActive]     = Accent;

        // ImGui >= 1.90.9 names (older: TabActive / TabUnfocused / TabUnfocusedActive)
        c[ImGuiCol_Tab]                  = Background;
        c[ImGuiCol_TabHovered]           = Control;
        c[ImGuiCol_TabSelected]          = Control;
        c[ImGuiCol_TabSelectedOverline]  = Accent;
        c[ImGuiCol_TabDimmed]            = Background;
        c[ImGuiCol_TabDimmedSelected]    = Control;
        c[ImGuiCol_TabDimmedSelectedOverline] = Accent;

        c[ImGuiCol_TableHeaderBg]        = Panel;
        c[ImGuiCol_TableBorderStrong]    = Divider;
        c[ImGuiCol_TableBorderLight]     = RowDivider;
        c[ImGuiCol_TableRowBg]           = Hex(0x000000, 0.0f);
        c[ImGuiCol_TableRowBgAlt]        = Hex(0xFFFFFF, 0.02f);

        c[ImGuiCol_TextSelectedBg]       = Hex(0xF2B544, 0.30f);
        c[ImGuiCol_DragDropTarget]       = Accent;
        c[ImGuiCol_ModalWindowDimBg]     = Hex(0x000000, 0.55f);

#ifdef IMGUI_HAS_DOCK
        c[ImGuiCol_DockingPreview]       = Hex(0xF2B544, 0.35f);
        c[ImGuiCol_DockingEmptyBg]       = Background;
#endif
    }
}
