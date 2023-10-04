#pragma once
#include "pch.h"

namespace seethe
{
class Timer
{
public:
	Timer();
	Timer(const Timer&) = default;
	Timer(Timer&&) = default;
	Timer& operator=(const Timer&) = default;
	Timer& operator=(Timer&&) = default;

	ND float TotalTime() const noexcept; // in seconds
	ND float DeltaTime() const noexcept; // in seconds

	void Reset(); // Call before message loop.
	void Start(); // Call when unpaused.
	void Stop();  // Call when paused.
	void Tick();  // Call every frame.

private:
	double m_secondsPerCount;
	double m_deltaTime;

	__int64 m_baseTime;
	__int64 m_pausedTime;
	__int64 m_stopTime;
	__int64 m_prevTime;
	__int64 m_currTime;

	bool m_stopped;
};
}