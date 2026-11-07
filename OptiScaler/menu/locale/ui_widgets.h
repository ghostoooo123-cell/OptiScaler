#pragma once

// Drop-in wrappers for the overlay. They translate user-visible text and leave
// ImGui calls that are pure ids (BeginTable, PushID, "##..." columns) alone.

#include "ui_locale.h"

#include <cstdarg>
#include <imgui/imgui.h>

namespace UiWidgets
{
inline void Text(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    ImGui::TextV(UiTr(fmt), args);
    va_end(args);
}

inline void TextColored(const ImVec4& col, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    ImGui::TextColoredV(col, UiTr(fmt), args);
    va_end(args);
}

inline void TextDisabled(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    ImGui::TextDisabledV(UiTr(fmt), args);
    va_end(args);
}

inline void TextWrapped(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    ImGui::TextWrappedV(UiTr(fmt), args);
    va_end(args);
}

inline void SetTooltip(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    ImGui::SetTooltipV(UiTr(fmt), args);
    va_end(args);
}

inline void TextLinkOpenURL(const char* label, const char* url = nullptr)
{
    ImGui::TextLinkOpenURL(UiLabel(label), url);
}

inline bool Button(const char* label, const ImVec2& size = ImVec2(0, 0)) { return ImGui::Button(UiLabel(label), size); }

inline bool Checkbox(const char* label, bool* v) { return ImGui::Checkbox(UiLabel(label), v); }

inline bool RadioButton(const char* label, bool active) { return ImGui::RadioButton(UiLabel(label), active); }

inline bool RadioButton(const char* label, int* v, int v_button)
{
    return ImGui::RadioButton(UiLabel(label), v, v_button);
}

inline bool Begin(const char* name, bool* p_open = nullptr, ImGuiWindowFlags flags = 0)
{
    return ImGui::Begin(UiLabel(name), p_open, flags);
}

inline bool BeginCombo(const char* label, const char* preview_value, ImGuiComboFlags flags = 0)
{
    return ImGui::BeginCombo(UiLabel(label), preview_value ? UiTr(preview_value) : preview_value, flags);
}

inline bool Selectable(const char* label, bool selected = false, ImGuiSelectableFlags flags = 0,
                       const ImVec2& size = ImVec2(0, 0))
{
    return ImGui::Selectable(UiLabel(label), selected, flags, size);
}

inline void SeparatorText(const char* label) { ImGui::SeparatorText(UiLabel(label)); }

inline bool TreeNode(const char* label) { return ImGui::TreeNode(UiLabel(label)); }

inline bool SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format = "%.3f",
                        ImGuiSliderFlags flags = 0)
{
    return ImGui::SliderFloat(UiLabel(label), v, v_min, v_max, format != nullptr ? UiTr(format) : format, flags);
}

inline bool SliderInt(const char* label, int* v, int v_min, int v_max, const char* format = "%d",
                      ImGuiSliderFlags flags = 0)
{
    return ImGui::SliderInt(UiLabel(label), v, v_min, v_max, format != nullptr ? UiTr(format) : format, flags);
}

inline bool InputFloat(const char* label, float* v, float step = 0.0f, float step_fast = 0.0f,
                       const char* format = "%.3f", ImGuiInputTextFlags flags = 0)
{
    return ImGui::InputFloat(UiLabel(label), v, step, step_fast, format, flags);
}

inline bool InputInt(const char* label, int* v, int step = 1, int step_fast = 100, ImGuiInputTextFlags flags = 0)
{
    return ImGui::InputInt(UiLabel(label), v, step, step_fast, flags);
}

inline bool InputScalar(const char* label, ImGuiDataType data_type, void* p_data, const void* p_step = nullptr,
                        const void* p_step_fast = nullptr, const char* format = nullptr, ImGuiInputTextFlags flags = 0)
{
    return ImGui::InputScalar(UiLabel(label), data_type, p_data, p_step, p_step_fast, format, flags);
}

inline bool ColorEdit3(const char* label, float col[3], ImGuiColorEditFlags flags = 0)
{
    return ImGui::ColorEdit3(UiLabel(label), col, flags);
}

inline bool Combo(const char* label, int* current_item, const char* const items[], int items_count,
                  int popup_max_height_in_items = -1)
{
    const char* translated[64];
    const char* const* used = items;
    if (items != nullptr && items_count > 0 && items_count <= 64)
    {
        for (int i = 0; i < items_count; ++i)
            translated[i] = UiLabel(items[i]);
        used = translated;
    }
    return ImGui::Combo(UiLabel(label), current_item, used, items_count, popup_max_height_in_items);
}

inline ImVec2 CalcTextSize(const char* text, const char* text_end = nullptr, bool hide_text_after_double_hash = false,
                           float wrap_width = -1.0f)
{
    return ImGui::CalcTextSize(UiTr(text), text_end, hide_text_after_double_hash, wrap_width);
}
} // namespace UiWidgets
