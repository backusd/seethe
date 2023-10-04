#pragma once
#include "pch.h"



namespace seethe
{
namespace log
{
	void error(const std::string& msg) noexcept;
	void warn(const std::string& msg) noexcept;
	void info(const std::string& msg) noexcept;
	void trace(const std::string& msg) noexcept;
}
}

#define LOG_ERROR(fmt, ...) seethe::log::error(std::format(fmt, __VA_ARGS__))
#define LOG_WARN(fmt, ...) seethe::log::warn(std::format(fmt, __VA_ARGS__))
#define LOG_INFO(fmt, ...) seethe::log::info(std::format(fmt, __VA_ARGS__))
#define LOG_TRACE(fmt, ...) seethe::log::trace(std::format(fmt, __VA_ARGS__))