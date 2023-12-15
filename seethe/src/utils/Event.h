#pragma once
#include "pch.h"

using EventHandler = std::function<void()>;
using EventHandlers = std::vector<EventHandler>;

namespace seethe
{
static constexpr void InvokeHandlers(const EventHandlers& handlers) noexcept
{
	std::for_each(handlers.begin(), handlers.end(), [](const EventHandler& h) { h(); });
}
}