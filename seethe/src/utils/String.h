#pragma once
#include "pch.h"



namespace seethe
{
ND std::wstring s2ws(const std::string& str) noexcept;
ND std::string ws2s(const std::wstring& wstr) noexcept;
}