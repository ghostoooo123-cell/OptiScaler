#if defined(_MSC_VER)
#include "pch.h"
#endif

#include "ui_locale.h"

#include <cctype>
#include <cstring>
#include <deque>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace
{
struct Entry
{
    const char* en;
    const char* zh;
};

const Entry kEntries[] = {
#include "zh_cn_catalog.inc"
};

constexpr int kEntryCount = (int) (sizeof(kEntries) / sizeof(kEntries[0]));

bool IsPrintfConversion(char c)
{
    switch (c)
    {
    case 'd':
    case 'i':
    case 'u':
    case 'o':
    case 'x':
    case 'X':
    case 'f':
    case 'F':
    case 'e':
    case 'E':
    case 'g':
    case 'G':
    case 'a':
    case 'A':
    case 'c':
    case 's':
    case 'p':
    case 'n':
        return true;
    default:
        return false;
    }
}

bool IsLengthPrefix(std::string_view s, size_t i, size_t& consumed)
{
    if (i + 1 < s.size() && s[i] == 'h' && s[i + 1] == 'h')
    {
        consumed = 2;
        return true;
    }
    if (i + 1 < s.size() && s[i] == 'l' && s[i + 1] == 'l')
    {
        consumed = 2;
        return true;
    }
    if (i < s.size() && (s[i] == 'h' || s[i] == 'l' || s[i] == 'j' || s[i] == 'z' || s[i] == 't' || s[i] == 'L'))
    {
        consumed = 1;
        return true;
    }
    consumed = 0;
    return false;
}

// Returns false when a '%' is not a valid conversion. `out` receives the conversion texts in order.
bool CollectPrintfSpecs(std::string_view s, std::vector<std::string>& out)
{
    out.clear();
    for (size_t i = 0; i < s.size(); ++i)
    {
        if (s[i] != '%')
            continue;

        size_t start = i;
        ++i;
        if (i >= s.size())
            return false;
        if (s[i] == '%')
        {
            out.emplace_back("%%");
            continue;
        }

        while (i < s.size() && (s[i] == '-' || s[i] == '+' || s[i] == ' ' || s[i] == '#' || s[i] == '0'))
            ++i;

        if (i < s.size() && s[i] == '*')
            ++i;
        else
            while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i])))
                ++i;

        if (i < s.size() && s[i] == '.')
        {
            ++i;
            if (i < s.size() && s[i] == '*')
                ++i;
            else
                while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i])))
                    ++i;
        }

        size_t length = 0;
        IsLengthPrefix(s, i, length);
        i += length;

        if (i >= s.size() || !IsPrintfConversion(s[i]))
            return false;

        out.emplace_back(s.substr(start, i - start + 1));
    }
    return true;
}

bool SamePrintfSpecs(std::string_view a, std::string_view b)
{
    std::vector<std::string> left;
    std::vector<std::string> right;
    if (!CollectPrintfSpecs(a, left) || !CollectPrintfSpecs(b, right))
        return false;
    return left == right;
}

std::string_view VisiblePortion(std::string_view text, std::string_view& suffix)
{
    const size_t pos = text.find("##");
    if (pos == std::string_view::npos)
    {
        suffix = {};
        return text;
    }
    suffix = text.substr(pos);
    return text.substr(0, pos);
}

// UTF-8 byte order. strcmp is signed on some platforms and would miss keys such as the euro sign.
int CompareUtf8(const char* left, const char* right)
{
    const auto* a = reinterpret_cast<const unsigned char*>(left);
    const auto* b = reinterpret_cast<const unsigned char*>(right);
    while (*a != 0 && *a == *b)
    {
        ++a;
        ++b;
    }
    return int(*a) - int(*b);
}

const char* LookupZh(std::string_view english)
{
    const std::string key(english);
    int lo = 0;
    int hi = kEntryCount;
    while (lo < hi)
    {
        const int mid = lo + (hi - lo) / 2;
        const int cmp = CompareUtf8(kEntries[mid].en, key.c_str());
        if (cmp == 0)
            return kEntries[mid].zh;
        if (cmp < 0)
            lo = mid + 1;
        else
            hi = mid;
    }
    return nullptr;
}

struct Intern
{
    std::deque<std::string> storage;
    std::unordered_map<std::string, const char*> cache;

    const char* Keep(const std::string& key, std::string value)
    {
        const auto found = cache.find(key);
        if (found != cache.end())
            return found->second;
        storage.push_back(std::move(value));
        const char* pointer = storage.back().c_str();
        cache.emplace(key, pointer);
        return pointer;
    }
};

Intern& Store()
{
    static Intern intern;
    return intern;
}

UiLocale::Language gLanguage = UiLocale::Language::English;
std::string gTag = "en";

std::string LowerAscii(const char* tag)
{
    std::string out;
    if (tag == nullptr)
        return out;
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(tag); *p != 0; ++p)
        out.push_back(static_cast<char>(std::tolower(*p)));
    return out;
}
}

namespace UiLocale
{
Language Current() { return gLanguage; }

bool IsZhCN() { return gLanguage == Language::ZhCN; }

const char* LanguageTag() { return gTag.c_str(); }

void SetLanguage(const char* tag)
{
    const std::string lower = LowerAscii(tag);
    if (lower == "zh" || lower == "zh-cn" || lower == "zh_cn" || lower == "chinese" || lower == "cn" ||
        lower == "chs" || lower == "zh-hans")
    {
        gLanguage = Language::ZhCN;
        gTag = "zh-CN";
        return;
    }

    gLanguage = Language::English;
    gTag = "en";
}

const char* Tr(const char* text)
{
    if (text == nullptr || !IsZhCN())
        return text;

    std::string_view suffix;
    const std::string_view visible = VisiblePortion(text, suffix);
    if (visible.empty())
        return text;

    const char* translated = LookupZh(visible);
    if (translated == nullptr)
        return text;
    if (std::strchr(translated, '#') != nullptr)
        return text;
    if (!SamePrintfSpecs(visible, translated))
        return text;
    if (suffix.empty() && std::strcmp(translated, text) == 0)
        return text;
    if (suffix.empty())
        return translated;

    std::string combined(translated);
    combined.append(suffix.data(), suffix.size());
    return Store().Keep(text, std::move(combined));
}

const char* Label(const char* label)
{
    if (label == nullptr || !IsZhCN() || label[0] == '\0')
        return label;

    std::string_view suffix;
    const std::string_view visible = VisiblePortion(label, suffix);
    if (visible.empty())
        return label;

    const char* translated = LookupZh(visible);
    if (translated == nullptr || std::strcmp(translated, std::string(visible).c_str()) == 0)
        return label;
    if (std::strchr(translated, '#') != nullptr)
        return label;
    if (!SamePrintfSpecs(visible, translated))
        return label;

    std::string combined = translated;
    if (suffix.size() >= 3 && suffix[0] == '#' && suffix[1] == '#' && suffix[2] == '#')
        combined.append(suffix.data(), suffix.size());
    else
    {
        combined += "###";
        combined += label;
    }
    return Store().Keep(std::string("label\n") + label, std::move(combined));
}

int CatalogSize() { return kEntryCount; }

void VisitTranslations(void (*fn)(const char* english, const char* translated, void* user), void* user)
{
    if (fn == nullptr)
        return;
    for (const Entry& entry : kEntries)
        fn(entry.en, entry.zh, user);
}
}
