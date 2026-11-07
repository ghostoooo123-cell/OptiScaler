#pragma once

// Unofficial in-game overlay localization. English strings stay the lookup keys.
// Missing translations and the English language both return the original pointer,
// so widget IDs and printf text stay identical to upstream when Language=en.

namespace UiLocale
{
enum class Language
{
    English,
    ZhCN
};

Language Current();
bool IsZhCN();
const char* LanguageTag();

// Accepts "en", "zh-CN", "zh", "zh_cn", "chinese". Anything else falls back to English.
void SetLanguage(const char* tag);

// Display text and printf formats. The returned pointer is stable for the process lifetime
// when a translation is used. The original pointer is returned for English, unknown keys,
// empty visible text, and translations whose printf conversions do not match.
const char* Tr(const char* text);

// ImGui widget label. English and untranslated labels are returned unchanged, so the
// upstream widget id is preserved. A translated label is "translation###<original>",
// which keeps a language-independent id payload and does not use the translation as the id.
const char* Label(const char* label);

int CatalogSize();
void VisitTranslations(void (*fn)(const char* english, const char* translated, void* user), void* user);

// Merges a Simplified Chinese glyph source into the atlas. No-op when disabled or already merged.
// Implemented in ui_locale_font.cpp (Windows / ImGui build). Safe to call before the first frame.
void MergeCjkGlyphs(void* fontAtlas, bool enabled);
}

inline const char* UiTr(const char* text) { return UiLocale::Tr(text); }
inline const char* UiLabel(const char* label) { return UiLocale::Label(label); }
