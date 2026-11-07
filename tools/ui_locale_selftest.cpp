#include "menu/locale/ui_locale.h"

#include <cstdio>
#include <cstring>
#include <string>

static int gFailures = 0;

static void Expect(bool condition, const char* message)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAIL: %s\n", message);
        ++gFailures;
    }
}

int main()
{
    Expect(UiLocale::CatalogSize() > 0, "catalog is empty");
    Expect(std::strcmp(UiLocale::LanguageTag(), "en") == 0, "default language is English");

    const char* enable = "Enable";
    Expect(UiLocale::Tr(enable) == enable, "English Tr returns the original pointer");
    Expect(UiLocale::Label(enable) == enable, "English Label returns the original pointer");
    Expect(std::strcmp(UiLocale::Label("Active##2"), "Active##2") == 0, "English keeps explicit ## ids");

    UiLocale::SetLanguage("zh-cn");
    Expect(UiLocale::IsZhCN(), "zh-cn selects Simplified Chinese");
    Expect(std::strcmp(UiLocale::LanguageTag(), "zh-CN") == 0, "canonical tag is zh-CN");

    const char* translated = UiLocale::Tr("Enable");
    Expect(translated != enable, "Enable is translated");
    Expect(std::strcmp(translated, "启用") == 0, "Enable translates to 启用");
    Expect(std::strcmp(UiLocale::Tr("this string is not in the catalog"), "this string is not in the catalog") == 0,
           "unknown text is unchanged");

    const char* labeled = UiLocale::Label("Enable");
    Expect(std::strcmp(labeled, "启用###Enable") == 0, "translated label keeps the English id payload");
    Expect(UiLocale::Label("Enable") == labeled, "label pointer is stable");

    const char* disambiguated = UiLocale::Label("Active##2");
    const char* active = UiLocale::Tr("Active");
    Expect(std::strstr(disambiguated, "###Active##2") != nullptr, "explicit ## suffix stays in the id payload");
    Expect(std::strncmp(disambiguated, active, std::strlen(active)) == 0, "Active##2 shows the Active translation");

    const char* triple = UiLocale::Label("Language###UiLanguage");
    Expect(std::strstr(triple, "###UiLanguage") != nullptr, "existing ### id is preserved");

    const char* pureId = UiLocale::Label("##UiLanguage");
    Expect(std::strcmp(pureId, "##UiLanguage") == 0, "pure ## ids are not rewritten");

    const char* format = UiLocale::Tr("Update available: %s (current %s)");
    Expect(std::strstr(format, "%s") != nullptr, "format placeholders survive translation");
    Expect(std::strcmp(format, "有可用更新: %s（当前 %s）") == 0, "update banner is translated");

    const char* euro = UiLocale::Tr("Did you really just pay 70€ for this game?!");
    Expect(std::strcmp(euro, "你真的为这款游戏花了 70€ 吗？！") == 0, "non-ASCII euro key is found");
    const char* curly = UiLocale::Tr("It’s too blurry to go alone, take RCAS with you");
    Expect(std::strcmp(curly, "独自上路太糊了，带上 RCAS 吧") == 0, "curly apostrophe key is found");

    UiLocale::SetLanguage("en");
    Expect(!UiLocale::IsZhCN(), "switching back to English");
    Expect(UiLocale::Tr(enable) == enable, "English mode restores the original pointer");
    Expect(std::strcmp(UiLocale::Label("Active##2"), "Active##2") == 0, "English mode restores ## labels");

    UiLocale::SetLanguage("not-a-language");
    Expect(!UiLocale::IsZhCN(), "unknown tags fall back to English");

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d failure(s)\n", gFailures);
        return 1;
    }
    std::printf("ui_locale selftest passed (%d catalog entries)\n", UiLocale::CatalogSize());
    return 0;
}
