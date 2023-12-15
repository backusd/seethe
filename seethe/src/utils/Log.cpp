#include "Log.h"


#ifdef DEBUG
#include <iostream>
#endif

namespace seethe
{
namespace log
{
std::string app_current_time_and_date()
{
	try
	{
		auto const time = std::chrono::current_zone()->to_local(std::chrono::system_clock::now());
		return std::format("{:%X}", time);
	}
	catch (const std::runtime_error& e)
	{
		return std::format("Caught runtime error: {}", e.what());
	}
}

#if defined DEBUG

void error(std::string_view msg) noexcept
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 4);
	std::cout << "[ERROR " << app_current_time_and_date() << "] " << msg << '\n';
}
void warn(std::string_view msg) noexcept
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 6);
	std::cout << "[WARN  " << app_current_time_and_date() << "] " << msg << '\n';
}
void info(std::string_view msg) noexcept
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
	std::cout << "[INFO  " << app_current_time_and_date() << "] " << msg << '\n';
}
void trace(std::string_view msg) noexcept
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	std::cout << "[TRACE " << app_current_time_and_date() << "] " << msg << '\n';

}

#elif defined RELEASE

void error(std::string_view msg) noexcept
{
	std::string time = app_current_time_and_date();
	std::wstring w = std::format(L"[ERROR {}] {}\n", std::wstring(time.begin(), time.end()), std::wstring(msg.begin(), msg.end()));
	OutputDebugString(w.c_str());
}
void warn(std::string_view msg) noexcept
{
	std::string time = app_current_time_and_date();
	std::wstring w = std::format(L"[WARN  {}] {}\n", std::wstring(time.begin(), time.end()), std::wstring(msg.begin(), msg.end()));
	OutputDebugString(w.c_str());
}
void info(std::string_view msg) noexcept
{
	std::string time = app_current_time_and_date();
	std::wstring w = std::format(L"[INFO  {}] {}\n", std::wstring(time.begin(), time.end()), std::wstring(msg.begin(), msg.end()));
	OutputDebugString(w.c_str());
}
void trace(std::string_view msg) noexcept
{
	std::string time = app_current_time_and_date();
	std::wstring w = std::format(L"[TRACE {}] {}\n", std::wstring(time.begin(), time.end()), std::wstring(msg.begin(), msg.end()));
	OutputDebugString(w.c_str());
}

#else
#error Need to define log functions
#endif
}
}