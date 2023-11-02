#include "String.h"


namespace seethe
{
std::wstring s2ws(const std::string& str) noexcept
{
    // See here for where this came from: https://stackoverflow.com/questions/4804298/how-to-convert-wstring-into-string
    // 
    // Create a wstring with the same size as str
    std::wstring w(str.length(), 0);

    // Cast each character in str to a wchar_t and assign into the result wstring
    std::transform(str.begin(), str.end(), w.begin(), [](char c) -> wchar_t
        {
            return static_cast<wchar_t>(c);
        }
    );
    return w;
}

std::string ws2s(const std::wstring& wstr) noexcept
{
    // See here for where this came from: https://stackoverflow.com/questions/4804298/how-to-convert-wstring-into-string
    // 
    // Create a string with the same size as wstr
    std::string s(wstr.length(), 0);

    // Cast each character in wstr to a char (this removes compiler warnings) and assign into the result string
    std::transform(wstr.begin(), wstr.end(), s.begin(), [](wchar_t c) -> char
        {
            return static_cast<char>(c);
        }
    );
    return s;
}
}