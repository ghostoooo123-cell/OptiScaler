#include "pch.h"
#include "ui_locale.h"
#include "zh_cn_font_subset.h"
#include "Util.h"

#include <imgui/imgui.h>

#include <cstdio>
#include <cstring>
#include <string>

namespace
{
bool AtlasHasZhCn(ImFontAtlas* atlas)
{
    for (const ImFontConfig& src : atlas->Sources)
    {
        if (std::strstr(src.Name, "ZhCN") != nullptr)
            return true;
    }
    return false;
}

void RememberRanges(ImFontGlyphRangesBuilder& builder)
{
    const ImWchar extra[] = {
        0x00D7, // multiplication sign
        0x2014, // em dash
        0x2018, 0x2019, 0x201C, 0x201D,
        0x2026, // ellipsis
        0x3001, 0x3002, 0x300A, 0x300B, 0x300C, 0x300D, 0x3010, 0x3011,
        0xFF01, 0xFF08, 0xFF09, 0xFF0C, 0xFF1A, 0xFF1B, 0xFF1F,
    };
    for (ImWchar codepoint : extra)
        builder.AddChar(codepoint);

    UiLocale::VisitTranslations(
        [](const char*, const char* translated, void* user)
        {
            if (translated != nullptr && translated[0] != '\0')
                static_cast<ImFontGlyphRangesBuilder*>(user)->AddText(translated);
        },
        &builder);
}

bool MergeMemoryFont(ImFontAtlas* atlas, float size, const ImWchar* ranges)
{
    if (ZhCnFontSubsetSize < 100)
        return false;

    ImFontConfig cfg;
    cfg.MergeMode = true;
    cfg.FontDataOwnedByAtlas = false;
    cfg.PixelSnapH = true;
    cfg.OversampleH = 1;
    cfg.OversampleV = 1;
    cfg.GlyphRanges = ranges;
    std::snprintf(cfg.Name, IM_ARRAYSIZE(cfg.Name), "OptiScaler ZhCN");

    ImFont* merged = atlas->AddFontFromMemoryTTF(const_cast<unsigned char*>(ZhCnFontSubset), (int) ZhCnFontSubsetSize,
                                                 size, &cfg, ranges);
    return merged != nullptr;
}

bool MergeFileFont(ImFontAtlas* atlas, float size, const ImWchar* ranges, const std::wstring& path, int fontNo)
{
    if (!std::filesystem::exists(path))
        return false;

    const std::string narrow = wstring_to_string(path);
    ImFontConfig cfg;
    cfg.MergeMode = true;
    cfg.FontNo = fontNo;
    cfg.PixelSnapH = true;
    cfg.OversampleH = 1;
    cfg.OversampleV = 1;
    cfg.GlyphRanges = ranges;
    std::snprintf(cfg.Name, IM_ARRAYSIZE(cfg.Name), "OptiScaler ZhCN");

    ImFont* merged = atlas->AddFontFromFileTTF(narrow.c_str(), size, &cfg, ranges);
    return merged != nullptr;
}
} // namespace

void UiLocale::MergeCjkGlyphs(void* fontAtlas, bool enabled)
{
    if (!enabled || fontAtlas == nullptr)
        return;

    auto* atlas = static_cast<ImFontAtlas*>(fontAtlas);
    if (AtlasHasZhCn(atlas))
        return;

    if (atlas->Fonts.empty())
        atlas->AddFontDefault();
    if (atlas->Fonts.empty())
        return;

    static ImVector<ImWchar> ranges;
    if (ranges.empty())
    {
        ImFontGlyphRangesBuilder builder;
        RememberRanges(builder);
        builder.BuildRanges(&ranges);
    }

    ImFont* base = atlas->Fonts.back();
    const float size = base->DefaultSize > 1.0f ? base->DefaultSize : 14.0f;

    if (MergeMemoryFont(atlas, size, ranges.Data))
    {
        LOG_INFO("Unofficial zh-CN overlay font merged from embedded Noto Sans SC subset");
        return;
    }

    const std::filesystem::path besideDll = Util::DllPath().parent_path() / L"OptiScalerZhCN.ttf";
    if (MergeFileFont(atlas, size, ranges.Data, besideDll.wstring(), 0))
    {
        LOG_INFO("Unofficial zh-CN overlay font merged from OptiScalerZhCN.ttf");
        return;
    }

    const wchar_t* systemFonts[] = { L"C:\\Windows\\Fonts\\msyh.ttc", L"C:\\Windows\\Fonts\\msyh.ttf",
                                     L"C:\\Windows\\Fonts\\simhei.ttf", L"C:\\Windows\\Fonts\\simsun.ttc" };
    for (const wchar_t* path : systemFonts)
    {
        if (MergeFileFont(atlas, size, ranges.Data, path, 0))
        {
            LOG_INFO("Unofficial zh-CN overlay font merged from a Windows system font");
            return;
        }
    }

    LOG_WARN("Unofficial zh-CN overlay has no CJK font. Chinese labels will show as missing glyphs.");
}
