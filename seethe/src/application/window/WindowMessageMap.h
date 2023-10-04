#pragma once
#include "pch.h"

namespace seethe
{
class WindowMessageMap
{
public:
	WindowMessageMap() noexcept;
	WindowMessageMap(const WindowMessageMap&) = delete;
	WindowMessageMap(WindowMessageMap&&) = delete;
	WindowMessageMap& operator=(const WindowMessageMap&) = delete;
	WindowMessageMap& operator=(WindowMessageMap&&) = delete;

	ND std::string operator()(DWORD msg, LPARAM lParam, WPARAM wParam) const noexcept;

private:
	std::unordered_map<DWORD, std::string> map;
};

}