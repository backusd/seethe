#pragma once
#include "pch.h"



namespace seethe
{
namespace log
{
	void error(std::string_view msg) noexcept;
	void warn(std::string_view msg) noexcept;
	void info(std::string_view msg) noexcept;
	void trace(std::string_view msg) noexcept;
}
}

#define LOG_ERROR(fmt, ...) seethe::log::error(std::format(fmt, __VA_ARGS__))
#define LOG_WARN(fmt, ...) seethe::log::warn(std::format(fmt, __VA_ARGS__))
#define LOG_INFO(fmt, ...) seethe::log::info(std::format(fmt, __VA_ARGS__))
#define LOG_TRACE(fmt, ...) seethe::log::trace(std::format(fmt, __VA_ARGS__))